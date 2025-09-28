# Camera RAW Dynamic range measurement
# www.overfitting.net
# https://www.overfitting.net/2025/07/rango-dinamico-de-un-sensor-de-imagen.html
# Drive de Hugo: https://drive.google.com/drive/folders/1SYbGBnXorQpJdaZaeTbdinNTv4ks3nlo

library(tiff)
library(png)
library(Cairo)
library(Rcpp)


# KEYSTONE CORRECTION (SAME FOR ALL IMAGES)
# Undo distortion function
calculate_keystone = function(xu, yu, xd, yd, DIMX, DIMY) {
    # Calculate the k parameters that define a keystone correction
    # from 4 pairs of (xu,yu) coords + 4 pairs of (xd,yd) coords
    
    # NOTE: we swap the distorted and undistorted trapezoids because
    # we want to model the pixel per pixel mapping
    # FROM CORRECTED coords (DST) -> TO UNCORRECTED coords (ORG)
    # DIMX, DIMY not needed for calculations, just to plot the trapezoids
    
    # Solve 8 equations linear system: A * k = b -> k = inv(A) * b
    A=matrix(nrow=8, ncol=8)
    A[1,]=c(xd[1], yd[1], 1, 0,     0,     0, -xd[1]*xu[1], -yd[1]*xu[1])
    A[2,]=c(0,     0,     0, xd[1], yd[1], 1, -xd[1]*yu[1], -yd[1]*yu[1])
    A[3,]=c(xd[2], yd[2], 1, 0,     0,     0, -xd[2]*xu[2], -yd[2]*xu[2])
    A[4,]=c(0,     0,     0, xd[2], yd[2], 1, -xd[2]*yu[2], -yd[2]*yu[2])
    A[5,]=c(xd[3], yd[3], 1, 0,     0,     0, -xd[3]*xu[3], -yd[3]*xu[3])
    A[6,]=c(0,     0,     0, xd[3], yd[3], 1, -xd[3]*yu[3], -yd[3]*yu[3])
    A[7,]=c(xd[4], yd[4], 1, 0,     0,     0, -xd[4]*xu[4], -yd[4]*xu[4])
    A[8,]=c(0,     0,     0, xd[4], yd[4], 1, -xd[4]*yu[4], -yd[4]*yu[4])
    b=as.matrix(c(xu[1], yu[1], xu[2], yu[2], xu[3], yu[3], xu[4], yu[4]))
    k=solve(A, b)  # equivalent to inv(A) * b = solve(A) %*% b
    
    # Check keystone correction
    for (i in 1:4) print(undo_keystone_coords(xd[i], yd[i], k))
    
    # Plot trapezoids
    plot(c(xd, xd[1]), c(yd, yd[1]), type='l', col='red', asp=1,
         xlab='X', ylab='Y', xlim=c(1,DIMX), ylim=c(DIMY,1))
    lines(c(xu, xu[1]), c(yu, yu[1]), type='l', col='blue')
    for (i in 1:4) {
        lines(c(xd[i], xu[i]), c(yd[i], yu[i]), type='l', lty=3, col='darkgray')
    }
    abline(h=c(1,DIMY), v=c(1,DIMX))
    
    return (k)
}

undo_keystone_coords = function(xd, yd, k) {
    # Return the keystone correction of a pair (or list of pairs) of coords
    denom=k[7]*xd + k[8]*yd + 1
    xu=(k[1]*xd + k[2]*yd + k[3]) / denom
    yu=(k[4]*xd + k[5]*yd + k[6]) / denom
    return(c(xu, yu))  # return pair (xu, yu)
}

cppFunction('
NumericMatrix undo_keystone_cpp(NumericMatrix imgd, NumericVector k) {
  // Return the keystone correction of an image (matrix)
  int DIMX = imgd.ncol();  // cols = X
  int DIMY = imgd.nrow();  // rows = Y
  NumericMatrix imgc(DIMY, DIMX);  // output image, same size

  for (int x = 0; x < DIMX; x++) {
    for (int y = 0; y < DIMY; y++) {
      double denom = k[6] * x + k[7] * y + 1;
      int xu = round((k[0] * x + k[1] * y + k[2]) / denom);
      int yu = round((k[3] * x + k[4] * y + k[5]) / denom);
      
      if (xu >= 0 && xu < DIMX && yu >= 0 && yu < DIMY) {
        imgc(y, x) = imgd(yu, xu);
      }
    }
  }

  return imgc;
}
')


# SNR CALCULATION OVER THE PATCHES
cppFunction('
List analyze_patches(NumericMatrix imgcrop, int NCOLS, int NROWS,
    double patch_ratio, double MIN_SNR_dB) {
  // Optimize SNR calculation over the patches
  int DIMX = imgcrop.ncol();
  int DIMY = imgcrop.nrow();
  
  std::vector<double> Signal;
  std::vector<double> Noise;

  for (int i = 1; i <= NCOLS; i++) {
    for (int j = 1; j <= NROWS; j++) {
      // Define dimensions of sub-patch to read
      double x1 = (i - 1) * DIMX / NCOLS;
      double x2 = i * DIMX / NCOLS;
      double EMPTYX = (x2 - x1) * (1 - patch_ratio) / 2;
      x1 = round((i - 1) * DIMX / NCOLS + EMPTYX);
      x2 = round(i * DIMX / NCOLS - EMPTYX);
      
      double y1 = (j - 1) * DIMY / NROWS;
      double y2 = j * DIMY / NROWS;
      double EMPTYY = (y2 - y1) * (1 - patch_ratio) / 2;    
      y1 = round((j - 1) * DIMY / NROWS + EMPTYY);
      y2 = round(j * DIMY / NROWS - EMPTYY);     
      
      std::vector<double> values;

      for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
          if (y >= 0 && y < DIMY && x >= 0 && x < DIMX)
            values.push_back(imgcrop(y, x));
        }
      }

      if (values.size() == 0) continue;

      // Compute mean and stddev
      double sum = 0.0;
      for (double v : values) sum += v;
      double mean = sum / values.size();

      double var = 0.0;
      for (double v : values) var += (v - mean) * (v - mean);
      double stdev = sqrt(var / values.size());

      // Count saturated pixels
      int sat = 0;
      for (double v : values) if (v > 0.9) sat++;

      if (mean > 0 && 20 * log10(mean / stdev) >= MIN_SNR_dB && ((double)sat / values.size()) < 0.01) {
      // if (mean > 0 && ((double)sat / values.size()) < 0.1) {
        Signal.push_back(mean);
        Noise.push_back(stdev);

        // Draw black inner rectangle
        for (int y = y1; y <= y2; y++) {
          if (x1 >= 0 && x1 < DIMX) imgcrop(y, x1) = 0;
          if (x2 >= 0 && x2 < DIMX) imgcrop(y, x2) = 0;
        }
        for (int x = x1; x <= x2; x++) {
          if (y1 >= 0 && y1 < DIMY) imgcrop(y1, x) = 0;
          if (y2 >= 0 && y2 < DIMY) imgcrop(y2, x) = 0;
        }

        // Draw white outer rectangle
        for (int y = y1; y <= y2; y++) {
          if (x1-1 >= 0 && x1-1 < DIMX) imgcrop(y, x1-1) = 1;
          if (x2+1 >= 0 && x2+1 < DIMX) imgcrop(y, x2+1) = 1;
        }
        for (int x = x1; x <= x2; x++) {
          if (y1-1 >= 0 && y1-1 < DIMY) imgcrop(y1-1, x) = 1;
          if (y2+1 >= 0 && y2+1 < DIMY) imgcrop(y2+1, x) = 1;
        }
      }
    }
  }

  return List::create(
    Named("Signal") = Signal,
    Named("Noise") = Noise,
    Named("imgcrop") = imgcrop
  );
}
')


# FUNCTION QUANTILE
cppFunction('
    double quantilecpp(Rcpp::NumericVector xx, double q = 0.5) {
    // q=0.5 -> median, q=0.25 -> first quartile, ...
    
    // Make a copy of xx because std::nth_element modifies the vector
    Rcpp::NumericVector x = Rcpp::clone(xx);
    
    std::size_t n = x.size() * q;
    std::nth_element(x.begin(), x.begin() + n, x.end());
    
    // Nearest rank quantile approximation
    return x[n];
    }
    ')


##################################################################
# MAGENTA CHART GENERATION

# This section implements the magenta chart creation functionality

# Chart format (suited to CAMERA, not to monitor)
Format=3/2  # 4/3  # 1
# Chart dimensions
DIMX=1920  # full HD width
DIMY=round(DIMX/Format)

# Effective area of chart
alpha=0.8

# Number of patches in chart (ideally nearly square patches)
NCOLS=6
NROWS=4

# Chart colours: UniWB for Canon 350D: R=162, G=64 y B=104 -> OPTIMIZE
R=162; G=64; B=104
RGBMAX=max(R,G,B)

# Gamma curve to otimize colour separation -> OPTIMIZE
invgamma=1.4  # nonlinear colour scale factor (inverse gamma)

chart=array(0, dim=c(DIMY, DIMX, 3))  # colour test chart

# Effective chart canvas inside white circles
DIMXc=DIMX*alpha
DIMYc=DIMY*alpha
OFFSETX=(DIMX-DIMXc)/2
OFFSETY=(DIMY-DIMYc)/2

# Draw magenta patches
WIDTH =DIMXc / (NCOLS+1)  # width of all patches in pixels (decimal)
HEIGHT=DIMYc / (NROWS+1)  # height of all patches in pixels (decimal)

val=seq(1, 0, length.out=NCOLS*NROWS)^invgamma
p=1
for (j in 1:NROWS) {
    for (i in 1:NCOLS) {
        x1=(i-1)*WIDTH  + OFFSETX + WIDTH/2
        x2= i   *WIDTH  + OFFSETX + WIDTH/2
        y1=(j-1)*HEIGHT + OFFSETY + HEIGHT/2
        y2= j   *HEIGHT + OFFSETY + HEIGHT/2
        patch=which(row(chart[,,1])>=y1 & row(chart[,,1])<=y2 &
                        col(chart[,,1])>=x1 & col(chart[,,1])<=x2)
        
        chart[,,1][patch]=val[p] * R/RGBMAX  # R
        chart[,,2][patch]=val[p] * G/RGBMAX  # G
        chart[,,3][patch]=val[p] * B/RGBMAX  # B
        p=p+1
    }
}


# Position of 4 white circles: top-left, bottom-left, bottom-right, top-right
x0=c(round(OFFSETX), round(OFFSETX),      round(DIMX-OFFSETX), round(DIMX-OFFSETX))
y0=c(round(OFFSETY), round(DIMY-OFFSETY), round(DIMY-OFFSETY), round(OFFSETY))

# Draw 4 blue lines
chart[y0[1]:y0[2], x0[1]:x0[2], 3]=0.75
chart[y0[2]:y0[3], x0[2]:x0[3], 3]=0.75
chart[y0[3]:y0[4], x0[3]:x0[4], 3]=0.75
chart[y0[4]:y0[1], x0[4]:x0[1], 3]=0.75

# Now draw 4 white circles. Radius: 1% of the whole Diagonal between circles
DIAG=(DIMX^2+DIMY^2)^0.5
RADIUS=DIAG*0.01  # Radius=1% of the whole Diagonal
AreaCircle=pi*RADIUS^2
AreaQuad=(DIMX/2)*(DIMY/2)
Quantile=AreaCircle/AreaQuad
THRESHOLD=1-Quantile/4  # THESHOLD to be used for quantile on corner detection

for (i in 1:4) {
    indices=which( ((row(chart[,,1])-y0[i])/RADIUS)^2 +
                       ((col(chart[,,1])-x0[i])/RADIUS)^2 < 1 )
    chart[,,1][indices]=1
    chart[,,2][indices]=1 
    chart[,,3][indices]=1 
}

# Write chart
CHARTNAME=paste0("testchart_", NCOLS, "x", NROWS, "_",
                 round(Format,2), "_", invgamma*10, ".png")
writePNG(chart, CHARTNAME)



##################################################################
# DYNAMIC RANGE ANALYSIS

################################
# RANGO CLI PARAMETERS

# This section tries to mimic rango CLI parameters to implement them in code


# --patch-ratio -r <float>
patch_ratio=0.5  # portion of width/height used on each patch

# --drnormalization-mpx -m <float>
normalize=FALSE  # TRUE
drnormalization_mpx=8  # 8Mpx normalization

# Number of patches in test chart
NCOLS=6
NROWS=4

# --chart-coords x1 y1 x2 y2 x3 y3 x4 y4
# Test chart defined by 4 corners: (x1,y1), (x2,y2), (x3,y3), (x4,y4)
# being (0,0) the coordinates of top-left corner
chartcoords=FALSE  # TRUE
# Example: distorted points (source) from Sony A7 II
xchart=c(1097, 1500, 5077, 4682)  # no need to be top-left, bottom-left, bottom-right, top-right
ychart=c(1152, 3396, 2800, 568)  # they will be re-ordered


################################

# 0. BLACK AND SAT POINTS CALCULATION FROM DARKFRAME AND BLOWN RAW FILES

# It is mandatory to use accurate BLACK (especially), and accurate SAT values
# to properly locate each pixel's level vs saturation

# dcraw -v -D -t 0 -4 -T BLACK.DNG SAT.DNG
imgblack=readTIFF("BLACK.tiff", as.is=TRUE)  # read unmodified integer RAW data
imgsat=readTIFF("SAT.tiff", as.is=TRUE)  # read unmodified integer RAW data

# Per-channel deep analysis of BLACK levels
for (rawchan in c('R', 'G1', 'G2', 'B')) {
    if (rawchan=='R') {
        imgBayerB=imgblack[row(imgblack)%%2 & col(imgblack)%%2]
        imgBayerS=imgsat[row(imgsat)%%2 & col(imgsat)%%2]
    } else if (rawchan=='G1') {
        imgBayerB=imgblack[row(imgblack)%%2 & !col(imgblack)%%2]
        imgBayerS=imgsat[row(imgsat)%%2 & !col(imgsat)%%2]
    } else if (rawchan=='G2') {
        imgBayerB=imgblack[!row(imgblack)%%2 & col(imgblack)%%2]
        imgBayerS=imgsat[!row(imgsat)%%2 & col(imgsat)%%2]
    } else if (rawchan=='B') {
        imgBayerB=imgblack[!row(imgblack)%%2 & !col(imgblack)%%2]
        imgBayerS=imgsat[!row(imgsat)%%2 & !col(imgsat)%%2]
    }

    print(paste0(rawchan, " BLACK: median=",
                 median(imgBayerB), ", mean=", mean(imgBayerB)))
    freq_table=table(as.vector(imgBayerB))
    write.csv2(freq_table, paste0("BLACK_freq_", rawchan, ".csv"))

    print(paste0(rawchan, " SAT: median=",
                 median(imgBayerS), ", mean=", mean(imgBayerS)))
    freq_table=table(as.vector(imgBayerS))
    write.csv2(freq_table, paste0("SAT_freq_", rawchan, ".csv"))
}

BLACK=mean(imgblack)  # 512.178 (Sony A7 IV) / 254.85 (Olympus OM-1)
BLACKV=c(512.177394540242, 512.154956138962, 512.143174113528, 512.236610063994)
SAT=median(imgsat)  # 16383


# In the end we can use this approximation for testing purposes
BLACK=512
SAT=16383


################################

# 1. EXTRACT RAW DATA AS 16-bit TIFF FILES READ THEM AND NORMALIZE

# dcraw -v -D -t 0 -4 -T *.DNG  # replace DNG for any camera RAW format

filepath=getwd()
filenames <- list.files(
    path = filepath,
    pattern = "^iso.*\\.tiff$",
    ignore.case = TRUE,
    full.names = FALSE
)

filenamesISO=gsub(".tiff", "", filenames) 
filenamesISO=toupper(gsub("^iso0*", "iso", filenamesISO))

ZOOM=1
NAME=paste0("SNRcurves_patchratio", patch_ratio, "_",
    ifelse(normalize, paste0(drnormalization_mpx, "Mpx"), "perpixel"), "_R.png")
CairoPNG(NAME, width=1920*ZOOM, height=1080*ZOOM)  # HQ Full HD curves

# Loop through all files
N=length(filenames)  # number of RAW files to process
for (image in 1:N) {
    NAME=filenames[image]
    cat(paste0('Processing "', NAME, '"...\n'))
    
    # Read RAW data
    img=readTIFF(NAME, as.is=TRUE)  # read unmodified integer RAW data

    # Normalize to floating point 0..1 range (negative values allowed and needed)
    img=img-BLACK
    # img=img-BLACKV[1]  # in case we want to differentiate black point per channel
    img=img/(SAT-BLACK)


################################
    
# 2. EXTRACT INDIVIDUAL RAW CHANNEL(S) AND APPLY KEYSTONE CORRECTION
    
    # Correct keystone distortion
    if (image==1) {  # all those things that we do only once
        # Obtain camera resolution in Mpx
        camresolution_mpx=nrow(img)*ncol(img)/1e6
        
        # Calculate k for ALL keystone corrections
        # Obtain circles coordinates from G1 channel
        # IMPORTANT NOTE: some cameras are GB/RG
        # on those cameras the G channel needs to be derived from metadata
        imgBayer=img[row(img)%%2 & !col(img)%%2]  # G1
        imgBayer[imgBayer<0]=0

        # imgBayer is a grayscale matrix with half dimensions as the RAW file
        dim(imgBayer)=dim(img)/2
        DIMX=ncol(imgBayer)
        DIMY=nrow(imgBayer)
        
        # Save G1 channel (used to detect keystone correction corners)
        imgsave=imgBayer/max(imgBayer)  # ETTR data
        imgsave[imgsave<0]=0  # clip below 0 values
        writePNG(imgsave^(1/2.2), paste0(NAME, "_G1chan.png"))
        rm(imgsave)

        # Perform keystone distortion correction
        xu=c(NA, NA, NA, NA)  # (xu,yu) are the 'undistorted' original points
        yu=c(NA, NA, NA, NA)
        if (chartcoords) {  # chart coordinates were provided in xchart, ychart
            # Reorder provided coordinates according to:
            # top-left, bottom-left, bottom-right, top-right
            pos1=which.min(xchart+ychart); xu[1]=xchart[pos1]; yu[1]=ychart[pos1]
            pos3=which.max(xchart+ychart); xu[3]=xchart[pos3]; yu[3]=ychart[pos3]
            pos2=which.min(xchart/ychart); xu[2]=xchart[pos2]; yu[2]=ychart[pos2]
            pos4=which.max(xchart/ychart); xu[4]=xchart[pos4]; yu[4]=ychart[pos4]
            
            xu=round(xu / 2)  # divide by 2 to be used on G1 half sized RAW data
            yu=round(yu / 2)
        } else {  # automatic corner detection
            for (sector in 1:4) {  # loop through 4 sectors (=quadrants)
                # 1: top-left, 2: bottom-left, 3: bottom-right, 4: top-right
                if (sector==1) imgsector=imgBayer[1:round(DIMY/2), 1:round(DIMX/2)]
                if (sector==2) imgsector=imgBayer[round(DIMY/2+1):DIMY, 1:round(DIMX/2)]
                if (sector==3) imgsector=imgBayer[round(DIMY/2+1):DIMY, round(DIMX/2+1):DIMX]
                if (sector==4) imgsector=imgBayer[1:round(DIMY/2), round(DIMX/2+1):DIMX]
    
                # Threshold for top Quantile of brightest pixels
                # White circles radius: 1% of the whole Diagonal
                DIAG=(DIMX^2+DIMY^2)^0.5
                RADIUS=DIAG*0.01  # Radius=1% of the whole Diagonal
                AreaCircle=pi*RADIUS^2
                AreaQuad=(DIMX/2)*(DIMY/2)
                Quantile=AreaCircle/AreaQuad
                THRESHOLD=1-Quantile/4  # THESHOLD to be used for quantile on corner detection
                q <- quantile(imgsector, probs = THRESHOLD)
                
                # Coordinates of pixels above threshold
                coords <- which(imgsector >= q, arr.ind = TRUE)
                imgsector[coords]=1  # mark as 1 pixels that participated in calculation
                
                # Median coordinates, rounded
                center_y <- round(median(coords[, 1]))
                center_x <- round(median(coords[, 2]))
                
                # Draw cartesian lines on calculated coords
                imgsector[center_y, 1:round(DIMX/2)]=1-imgsector[center_y, 1:round(DIMX/2)]
                imgsector[1:round(DIMY/2), center_x]=1-imgsector[1:round(DIMY/2), center_x]
                
                # 1: top-left, 2: bottom-left, 3: bottom-right, 4: top-right
                if (sector==1) {
                    imgBayer[1:round(DIMY/2), 1:round(DIMX/2)]=imgsector
                    xu[sector]=center_x
                    yu[sector]=center_y
                }
                if (sector==2) {
                    imgBayer[round(DIMY/2+1):DIMY, 1:round(DIMX/2)]=imgsector
                    xu[sector]=center_x
                    yu[sector]=center_y+round(DIMY/2)
                }
                if (sector==3) {
                    imgBayer[round(DIMY/2+1):DIMY, round(DIMX/2+1):DIMX]=imgsector
                    xu[sector]=center_x+round(DIMX/2)
                    yu[sector]=center_y+round(DIMY/2)
                }
                if (sector==4) {
                    imgBayer[1:round(DIMY/2), round(DIMX/2+1):DIMX]=imgsector
                    xu[sector]=center_x+round(DIMX/2)
                    yu[sector]=center_y
                }
            }
            # Write imgBayer with coords detection
            writeTIFF(imgBayer, paste0(NAME, "_cornersdetection.tif"), bits.per.sample=16)
        }

        # Undistorted points (destination): (floating point values)
        # top-left
        xtl=(xu[1] + xu[2]) / 2
        ytl=(yu[1] + yu[4]) / 2
        # bottom-right
        xbr=(xu[3] + xu[4]) / 2
        ybr=(yu[2] + yu[3]) / 2
        
        xd=c(xtl, xtl, xbr, xbr)  # (xd,yd) are the 'distorted' destination points
        yd=c(ytl, ybr, ybr, ytl)
        
        keystone=calculate_keystone(xu, yu, xd, yd, DIMX, DIMY)
    }
    
    Signal=c()  # empty vectors for current image
    Noise=c()
    patches_used=c(0,0,0,0)  # samples count
    # Now loop through all 4 RAW channels adding data points
    for (rawchan in 1:1) {
        if (rawchan==1) imgBayer=img[row(img)%%2 & col(img)%%2]  # R
        if (rawchan==2) imgBayer=img[row(img)%%2 & !col(img)%%2]  # G1
        if (rawchan==3) imgBayer=img[!row(img)%%2 & col(img)%%2]  # G2
        if (rawchan==4) imgBayer=img[!row(img)%%2 & !col(img)%%2]  # B
        dim(imgBayer)=dim(img)/2
        DIMX=ncol(imgBayer)
        DIMY=nrow(imgBayer)

        # Correct keystone distortion
        imgc=undo_keystone_cpp(imgBayer, keystone)

################################
    
# 3. READ PATCHES TO FORM GRID AND COLLECT (EV,SNR) PAIRS
    
        # Crop patches area dropping corner marks (that are 0.5 patches away)
        if (chartcoords) {  # when specifying the chart coords there is no gap
            GAPX=0
            GAPY=0          
        } else {
            GAPX=(xbr-xtl) / (NCOLS+1) / 2
            GAPY=(ybr-ytl) / (NROWS+1) / 2
        }
        imgcrop=imgc[round(ytl+GAPY):round(ybr-GAPY), round(xtl+GAPX):round(xbr-GAPX)]
        MAXCROP=max(imgcrop)

        # Analyze imgcrop divided in NCOLS x NROWS patches leaving patch_ratio
        MIN_SNR_dB = -10; MIN_SNR_dB = -90
        if (normalize) MIN_SNR_dB = MIN_SNR_dB - 20*log10((camresolution_mpx / drnormalization_mpx)^(1/2))
        
        calc=analyze_patches(imgcrop, NCOLS, NROWS, patch_ratio, MIN_SNR_dB)
        Signal=c(Signal, calc$Signal)  # add signal values to Signal vector
        Noise=c(Noise, calc$Noise)  # add noise values to Noise vector
        patches_used[rawchan]=length(calc$Signal)  # total number of samples used in rawchan      
        
        # Save normalized and gamma corrected crop with used patches overlapped on the chart
        if (image==1 && rawchan==1) {  # do it just once, although it can be calculated for every image/chan
            imgsave=calc$imgcrop/MAXCROP  # ETTR data
            imgsave[imgsave<0]=0  # clip below 0 values
            imgsave[imgsave>1]=1  # clipp saturated values
            writePNG(imgsave^(1/2.2), paste0("printpatches_RAW", rawchan,
                        "_IMG", image, '_', NAME, ".png"))
            rm(imgsave)
        }
        
    }  # end loop through RAW channels
    
    # Now order ALL 4 channels samples from lower to higher SNR values (to plot beautifully)
    SNR=Signal/Noise
    neworder=order(SNR)  # SNR will be the independent variable in the regression curve
    Signal=Signal[neworder]
    Noise=Noise[neworder]
    SNR=SNR[neworder]
    # SNR normalization to drnormalization_mpx Mpx
    # Linear: SNR_norm = SNR_perpixel * (Mpx / 8)^(1/2)
    # Log:    SNR_norm dB = SNR_perpixel dB + 20 * log10[(Mpx / 8)^(1/2)]
    if (normalize) SNR = SNR * (camresolution_mpx / drnormalization_mpx)^(1/2)

    # SNR curves in dB: we'll plot blue scatter points and red curves
    if (image==1) {
        plot(log2(Signal), 20*log10(SNR), xlim=c(-18,0), ylim=c(-20,30),
             pch=16, cex=0.5, col='blue',
             # main='SNR curves - Olympus OM-1',
             main='SNR curves - Sony A7 IV',
             xlab='RAW exposure (EV)', ylab='SNR (dB)')
        abline(h=c(0,12), lty=2)
        abline(v=seq(-14,0,1), lty=2, col='gray')
        axis(side=1, at=-14:0)
    } else {
        points(log2(Signal), 20*log10(SNR), pch=16, cex=0.5, col='blue')        
    }
    
    # # SNR curves in EV
    # plot(log2(Signal), log2(SNR), xlim=c(-14,0), ylim=c(-2,4), 
    #      pch=16, cex=0.5, col='blue',
    #      main=paste0('SNR curves\nOlympus OM-1 at ', NAME),
    #      xlab='RAW exposure (EV)', ylab='SNR (EV)')
    # abline(h=c(0,2), lty=2)
    # abline(v=seq(-14,0,1), lty=2, col='gray')

    
################################

# 4. APPROXIMATION CURVES TO CALCULATE DR VALUES
    
    # Soft cubic splines approximation
    # https://stackoverflow.com/questions/37528590/r-smooth-spline-smoothing-spline-is-not-smooth-but-overfitting-my-data
    # NOTES:
    #   Inverted variables: Signal = f(SNR) -> DR = f(SNR threshold)
    #   Splines already in log transformed domains (softer derivatives)
    spline_fit=smooth.spline(20*log10(SNR), log2(Signal),
                             spar=0.5, nknots=10)  # spar controls smoothness
    valuesx=predict(spline_fit, 20*log10(SNR))$y
    valuesy=20*log10(SNR)
    lines(valuesx, valuesy, col='red', type='l')
    # Label each curve with its ISO value
    text(max(valuesx), max(valuesy)+0.5, labels=filenamesISO[image], col='red')
    
    # Now calculate the Dynamic Range for a given SNR threshold criteria
    TH_dB=c(12, 0)  # Photographic (12dB) and Engineering (0dB) DR
    DR_EV=c()
    for (threshold in 1:length(TH_dB)) {
        DR_EV=c(DR_EV, -predict(spline_fit, TH_dB[threshold])$y)
        xpos=-DR_EV[threshold]
        ypos=TH_dB[threshold]
        points(xpos, ypos, pch=3, cex=1.5)
        text(xpos, ypos+0.5,
             labels=paste0(round(DR_EV[threshold],1), "EV"))
    }

    # Create dataframe with all calculated DR values
    if (image==1) {
        DR_df=data.frame(tiff_file=NAME,
                         DR_EV_12dB=DR_EV[1], DR_EV_0dB=DR_EV[2],
                         samples_R =patches_used[1],
                         samples_G1=patches_used[2],
                         samples_G2=patches_used[3],
                         samples_B =patches_used[4])        
    } else {
        new_row=data.frame(tiff_file=NAME,
                           DR_EV_12dB=DR_EV[1], DR_EV_0dB=DR_EV[2],
                           samples_R =patches_used[1],
                           samples_G1=patches_used[2],
                           samples_G2=patches_used[3],
                           samples_B =patches_used[4])
        DR_df=rbind(DR_df, new_row)
    }
}

# Label DR calculations
text(-16+0.1, 12+0.5, labels="Photographic DR (SNR>12dB)", adj=0)
text(-16+0.1,  0+0.5, labels="Engineering DR (SNR>0dB)", adj=0)

dev.off()

# Print calculated DR for each ISO
DR_df=DR_df[order(DR_df$tiff_file), ]
write.csv2(DR_df, paste0("results_SonyA7II.csv"))
print(DR_df)







##################################################################
# POLYNOMIC FITTING EXAMPLES

# Create (x,y) samples
set.seed(100)
dev1=0.2/2
dev2=100/2
x <- 1:10 + rnorm(length(x), sd=dev1)
y <- 1000-(15-x)^3 + rnorm(length(x), sd=dev2)  # noisy cubic data

# Fitting for y=f(x)
neworder=order(x)
x=x[neworder]
y=y[neworder]

# Fit cubic polynomial: y=f(x)
fit <- lm(y ~ poly(x=x, degree=3, raw=TRUE))
a=coef(fit)
# y_pred <- predict(fit)
x_new <- seq(min(x), max(x), length.out = 500)
y_new <- predict(fit, newdata = data.frame(x = x_new))

plot(x, y, main="Cubic polynomial fit y=f(x)", pch=19)
lines(x_new, y_new, col=rgb(1,0,0,0.1), lwd=12)
lines(x_new, a[1] + a[2]*x_new + a[3]*x_new^2 + a[4]*x_new^3, col='red')

xfwd=x_new; yfwd=y_new


# Fitting for x=f(y)
neworder=order(y)
x=x[neworder]
y=y[neworder]

# Fit cubic polynomial: x=f(y)
fit <- lm(x ~ poly(x=y, degree=3, raw=TRUE))
a=coef(fit)
# x_pred <- predict(fit)
y_new <- seq(min(y), max(y), length.out = 500)
x_new <- predict(fit, newdata = data.frame(y = y_new))

plot(x, y, main="Cubic polynomial fit x=f(y)", pch=19)
lines(x_new, y_new, col=rgb(0,0,1,0.2), lwd=12)
lines(a[1] + a[2]*y_new + a[3]*y_new^2 + a[4]*y_new^3, y_new, , col='blue')

xbkd=x_new; ybkd=y_new


# Plot both polynomials: y=f(x) and x=f(y)
plot(x, y, main="Forward vs Backward xubic polynomial fit comparison", pch=19)
lines(xfwd, yfwd, col="red", lwd=1)
lines(xbkd, ybkd, col="blue", lwd=1)



################################
# (Exposicion,SNR) example

# Data: (Exposicion, SNR) pairs
set.seed(10)
Exposicion <- 1:10-12 + rnorm(10, sd=0.15)
SNR <- (15000-(15-Exposicion)^3 + rnorm(10, sd=80))/400  # noisy cubic data
plot(Exposicion, SNR)

# Fit cubic polynomial: Exposicion=f(SNR)
# fit <- lm(y ~ poly(x, 3, raw=TRUE)) -> y=Exposicion, x=SNR
fit <- lm(Exposicion ~ poly(SNR, 3, raw=TRUE))  # cubic polynomial fit
Exposicion_pred <- predict(fit)
plot(Exposicion, SNR, main="Cubic polynomial fit", pch=19)
lines(Exposicion_pred, SNR, col="red", lwd=2)

RD <- -predict(fit, newdata = data.frame(SNR = c(0,12)))
abline(h=c(0,12), v=-RD,, lty='dotted')


