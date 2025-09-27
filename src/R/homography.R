library(Cairo)
library(tiff)
library(Rcpp)

# ==== Homography helpers ====

# Compute homography matrix H that maps a unit square (0–1 range) 
# into the given quadrilateral (quad) in image space.
homography_from_quad <- function(quad) {
    # source unit square coordinates
    src <- matrix(c(0,0, 1,0, 1,1, 0,1), ncol=2, byrow=TRUE)
    A <- matrix(0, 8, 8); b <- numeric(8)
    
    # build linear system for homography coefficients
    for (k in 1:4) {
        u <- src[k,1]; v <- src[k,2]
        x <- quad[k,1]; y <- quad[k,2]
        A[2*k-1,] <- c(u, v, 1, 0, 0, 0, -u*x, -v*x)
        A[2*k,  ] <- c(0, 0, 0, u, v, 1, -u*y, -v*y)
        b[2*k-1] <- x; b[2*k] <- y
    }
    # solve system and form 3×3 homography matrix
    h <- solve(A, b)
    H <- matrix(c(h, 1), 3, 3, byrow=TRUE)
    return(H)
}

# Apply homography H to a point (u,v) in unit square space
# → returns (x,y) in image space.
apply_H <- function(H, u, v) {
    p <- H %*% c(u, v, 1)
    c(p[1]/p[3], p[2]/p[3])
}

# Generate all grid corner points for a rows×cols grid mapped by H.
grid_points <- function(H, rows, cols) {
    out <- array(0, dim=c(rows+1, cols+1, 2))
    for (j in 0:rows) {
        for (i in 0:cols) {
            out[j+1, i+1, ] <- apply_H(H, i/cols, j/rows)
        }
    }
    out
}

# ==== Outer patch polygons ====

# Build polygons for each outer patch (the visible grid cells).
patch_polygons <- function(grid_img, rows, cols) {
    patches <- vector("list", rows*cols)
    k <- 1
    for (r in 1:rows) {
        for (c in 1:cols) {
            poly <- rbind(
                grid_img[r,   c,   ],   # top-left
                grid_img[r,   c+1, ],   # top-right
                grid_img[r+1, c+1, ],   # bottom-right
                grid_img[r+1, c,   ]    # bottom-left
            )
            patches[[k]] <- poly
            k <- k+1
        }
    }
    patches
}

# ==== Inner patch polygons following perspective ====

# Create shrunken inner patches within each grid cell.
# alphax, alphay define shrink factors in horizontal/vertical directions.
inner_patches <- function(H,grid_img, rows, cols, alphax=0.5, alphay=0.5) {
    inner <- vector("list", rows*cols)
    k <- 1
    for (r in 1:rows) {
        for (c in 1:cols) {
            # logical coordinates of outer patch
            u0 <- (c-1)/cols; u1 <- c/cols
            v0 <- (r-1)/rows; v1 <- r/rows
            uc <- (u0+u1)/2; vc <- (v0+v1)/2
            
            # shrink corners toward the patch center
            u_inner <- uc + (c(u0,u1,u1,u0) - uc) * alphax
            v_inner <- vc + (c(v0,v0,v1,v1) - vc) * alphay
            
            # map through homography into image space
            inner[[k]] <- t(mapply(function(u,v) apply_H(H,u,v), u_inner, v_inner))
            k <- k+1
        }
    }
    inner
}


# ==== Example usage ====

# Quadrilateral in image space that the grid should map onto
quad <- matrix(c(100,50,
                 800,120,
                 600,400,
                 300,350), ncol=2, byrow=TRUE)
rows <- 5; cols <- 6   # grid size

# Magenta Test chart
# Quadrilateral in image space that the grid should map onto
quad <- matrix(c(569-25,588,
                 746-25,1696+20,
                 2529,1372+15+20,
                 2340,292), ncol=2, byrow=TRUE)
rows <- 6; cols <- 4   # grid size



# Compute homography and grid structure
H <- homography_from_quad(quad)
grid_img <- grid_points(H, rows, cols)
patches <- patch_polygons(grid_img, rows, cols)

# Inner patches: keep 50% width, 60% height relative to outer patch
inner <- inner_patches(H, grid_img, rows, cols, alphax=0.5, alphay=0.6)

# ==== Plot to file ====
CairoPNG("Keystone_correction_NEW0.7.png", width=1920, height=1200)
    plot(NA, xlim=range(quad[,1]), ylim=range(quad[,2]),
         asp=1, xlab="x", ylab="y", main="Inner patches")
    
    # outer quadrilateral outline
    polygon(quad, border="red", lwd=4)
    
    # draw outer patches (blue, semi-transparent fill)
    for (poly in patches) polygon(poly, border="blue", col=rgb(0,0,1,0.1))
    
    # draw inner patches (dark gray border, semi-transparent fill)
    for (poly in inner) polygon(poly, border="darkgray", col=rgb(0,0,1,0.2))
    
    # plot outer grid corners (small black points)
    points(grid_img[,,1], grid_img[,,2], pch=19, col="black", cex=0.5)
    
    # plot inner patch corners (small black points)
    for (poly in inner) points(poly[,1], poly[,2], pch=19, cex=0.5, col="black")
dev.off()


############################
# Read pixel values on a grayscale image (integer 2D matrix)

point_in_poly <- function(px, py, poly) {
    # poly is an Nx2 matrix of [x,y] vertices (closed or open)
    n <- nrow(poly)
    inside <- FALSE
    j <- n
    for (i in 1:n) {
        if (((poly[i,2] > py) != (poly[j,2] > py)) &&
            (px < (poly[j,1] - poly[i,1]) * (py - poly[i,2]) /
             (poly[j,2] - poly[i,2]) + poly[i,1])) {
            inside <- !inside
        }
        j <- i
    }
    inside
}

# img is a grayscale matrix, e.g., img[row, col]
img=readTIFF("iso100_G1.tiff")
nr <- nrow(img)
nc <- ncol(img)

patch_means <- numeric(length(inner))
# Output matrix of patch IDs
patch_map <- matrix(0, nrow = nr, ncol = nc)

for (k in seq_along(inner)) {
    poly <- inner[[k]]  # 4x2 polygon
    
    # Restrict to bounding box for efficiency
    xmin <- max(1, floor(min(poly[,1])))
    xmax <- min(nc, ceiling(max(poly[,1])))
    ymin <- max(1, floor(min(poly[,2])))
    ymax <- min(nr, ceiling(max(poly[,2])))
    
    vals <- numeric(0)
    
    for (y in ymin:ymax) {
        for (x in xmin:xmax) {
            if (point_in_poly(x, y, poly)) {
                vals <- c(vals, img[y, x])
                patch_map[y, x] <- k
            }
        }
    }
    
    patch_means[k] <- if (length(vals) > 0) mean(vals) else NA
}

writeTIFF(patch_map/max(patch_map), "patch_map.tif")


####################
# C++ version

cppFunction('
List assign_and_stats(NumericMatrix img, List patches) {
    int nr = img.nrow();
    int nc = img.ncol();
    int P = patches.size();

    IntegerMatrix patch_map(nr, nc);
    NumericVector Signal(P);
    NumericVector Noise(P);
    IntegerVector Count(P);

    for (int k = 0; k < P; k++) {
        NumericMatrix poly = patches[k];

        double x1[4], y1[4], x2[4], y2[4];
        for (int i = 0; i < 4; i++) {
            x1[i] = poly(i, 0);
            y1[i] = poly(i, 1);
            x2[i] = poly((i + 1) % 4, 0);
            y2[i] = poly((i + 1) % 4, 1);
        }

        for (int y = 0; y < nr; y++) {
            for (int x = 0; x < nc; x++) {

                bool inside = true;
                for (int i = 0; i < 4; i++) {
                    double cross = (x + 1 - x1[i]) * (y2[i] - y1[i]) - (y + 1 - y1[i]) * (x2[i] - x1[i]);
                    if (cross < 0) { inside = false; break; }
                }

                if (inside) {
                    patch_map(y, x) = k + 1;

                    double val = img(y, x);
                    Signal[k]     += val;
                    Noise[k]      += val * val;
                    Count[k]      += 1;
                }
            }
        }
    }

    for (int k = 0; k < P; k++) {
        if (Count[k] > 0) {
            double mean = Signal[k] / Count[k];
            Signal[k] = mean;
            Noise[k] = sqrt((Noise[k] / Count[k]) - (mean * mean));  // stddev
        } else {
            Signal[k] = NA_REAL;
            Noise[k]  = NA_REAL;
        }
    }

    return List::create(
        Named("Signal") = Signal,
        Named("Noise")  = Noise,
        Named("patch_map") = patch_map
    );
}
')

result <- assign_and_stats(img, inner)

result$Signal     # mean per patch
result$Noise      # std deviation per patch
result$patch_map  # integer matrix with patch IDs

writeTIFF(result$patch_map/max(result$patch_map), "patch_map_cpp_V3.tif")
