# Camera RAW Dynamic range measurement
# www.overfitting.net
# https://www.overfitting.net/

library(tiff)
library(Cairo)


################################

# KEYSTONE CORRECTION (SAME FOR ALL IMAGES)

# Undo distortion function
keystone.correction = function(DIMX, DIMY) {
    # Distorted points (source)
    xu=c(119, 99, 2515, 2473)  # top-left, bottom-left, bottom-right, top-right
    yu=c(170, 1687, 1679, 158)
    
    # Undistorted points (destination):
    
    # top-left
    xtl=(xu[1] + xu[2]) / 2
    ytl=(yu[1] + yu[4]) / 2
    # bottom-right
    xbr=(xu[3] + xu[3]) / 2
    ybr=(yu[2] + yu[3]) / 2
    
    xd=c(xtl, xtl, xbr, xbr)
    yd=c(ytl, ybr, ybr, ytl)
    
    # NOTE: we swap the distorted and undistorted trapezoids because
    # we want to model the transformation
    # FROM CORRECTED coords (DST) -> TO UNCORRECTED coords (ORG)
    
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
    for (i in 1:4) print(undo.keystone(xd[i], yd[i], k))
    
    # Plot trapezoids
    plot(c(xd, xd[1]), c(yd, yd[1]), type='l', col='red', asp=1,
         xlab='X', ylab='Y', xlim=c(1,DIMX), ylim=c(DIMY,1))
    lines(c(xu, xu[1]), c(yu, yu[1]), type='l', col='blue')
    for (i in 1:4) {
        lines(c(xd[i], xu[i]), c(yd[i], yu[i]), type='l', lty=3, col='darkgray')
    }
    abline(h=c(1,DIMY), v=c(1,DIMX))
    
    return (list(k, c(xtl,ytl,xbr,ybr)))
}

undo.keystone = function(xd, yd, k) {
    xu=(k[1]*xd+k[2]*yd+k[3]) / (k[7]*xd+k[8]*yd+1)
    yu=(k[4]*xd+k[5]*yd+k[6]) / (k[7]*xd+k[8]*yd+1)
    return(c(xu, yu))  # return pair (xu, yu)
}


################################

# 1. SAVE RAW DATA AS 16-bit TIFF FILES
# dcraw -v -D -t 0 *.DNG  # replace DNG for any camera RAW format


################################

# 2. READ RAW VALUES FROM TIFF FILES AND NORMALIZE

BLACK=256  # Olympus OM-1 black level
SAT=4095  # Olympus OM-1 sat level

filepath=getwd()
filenames=list.files(path=filepath, pattern="\\.tiff$",  # pattern="\\.tif{1,2}$",
                      ignore.case=TRUE, full.names=FALSE)

CairoPNG("SNRcurves.png", width=1920, height=1080)  # HQ Full HD curves

N=length(filenames)  # number of RAW files to process
for (image in 1:N) {
    NAME=filenames[image]
    cat(paste0('Processing "', NAME, '" ...\n'))
    
    # Read RAW data
    img=readTIFF(NAME, native=FALSE, convert=FALSE, as.is=TRUE)
    # hist(img, breaks=800, main=paste0('"', NAME, '" RAW histogram'))
    # abline(v=BLACK, col='red')
    
    # Normalize to floating point 0..1 range (negative values are allowed)
    img=img-BLACK
    img=img/(SAT-BLACK)
    # hist(img, breaks=800)
    # abline(v=0, col='red')

    
################################
    
# 3. EXTRACT INDIVIDUAL RAW CHANNEL(S) AND APPLY KEYSTONE CORRECTION
    
    # Keep G1 channel
    imgBayer=img[row(img)%%2 & col(img)%%2]
    dim(imgBayer)=dim(img)/2

    # Correct keystone distortion
    DIMX=ncol(imgBayer)
    DIMY=nrow(imgBayer)
    if (image==1) {
        keystone=keystone.correction(DIMX, DIMY)
        k=unlist(keystone[1])  # calculate keystone correction
        coords=unlist(keystone[2])  # coords of corrected image
        xtl=coords[1]
        ytl=coords[2]
        xbr=coords[3]
        ybr=coords[4]
    }
    
    imgc=imgBayer*0
    for (x in 1:DIMX) {
        for (y in 1:DIMY) {
            xuyu=round(undo.keystone(x, y, k))
            if (xuyu[1]>=1 & xuyu[1]<=DIMX & xuyu[2]>=1 & xuyu[2]<=DIMY)
                imgc[y, x]=imgBayer[xuyu[2], xuyu[1]]  # nearest neighbour interp
        }
    }
    
    # Save corrected image
    imgsave=imgc
    imgsave[imgsave<0]=0
    writeTIFF(imgsave, paste0("correctedchart_", NAME, ".tif"), bits.per.sample=16)
    rm(imgsave)
    
    
################################
    
# 4. READ PATCHES TO FORM 7x11 GRID AND COLLECT (EV,SNR) PAIRS
    
    # Crop patches area
    imgcrop=imgc[round(ytl):round(ybr), round(xtl):round(xbr)]
    DIMX=ncol(imgcrop)
    DIMY=nrow(imgcrop)
    
    NCOLS=11
    NROWS=7
    SAFE=50
    Signal=c()
    Noise=c()
    
    for (i in 1:NCOLS) {
        for (j in 1:NROWS) {
            x1=round((i-1)*DIMX/NCOLS + SAFE)
            x2=round( i   *DIMX/NCOLS - SAFE)
            y1=round((j-1)*DIMY/NROWS + SAFE)
            y2=round( j   *DIMY/NROWS - SAFE)
            patch=which(row(imgcrop)>=y1 & row(imgcrop)<=y2 & 
                        col(imgcrop)>=x1 & col(imgcrop)<=x2)
            values=imgcrop[patch]
            S=mean(values)  # S=mean
            N=var(values)^0.5  # N=stdev
            
            # Ignore patches with negative average values, SNR < -10dB or
            # >1% of saturated/nonlinear (>90%) values
            if (S>0 & 20*log10(S/N) >= -10 & length(values[values>0.9])/length(values)<0.01) {
                Signal=c(Signal,S)
                Noise=c(Noise, N)
                
                imgcrop[y1:y2,x1]=0
                imgcrop[y1:y2,x2]=0
                imgcrop[y1,x1:x2]=0
                imgcrop[y2,x1:x2]=0
                
                imgcrop[y1:y2,(x1-1)]=1
                imgcrop[y1:y2,(x2+1)]=1
                imgcrop[(y1-1),x1:x2]=1
                imgcrop[(y2+1),x1:x2]=1
            }

        }
    }
    used_patches=length(Signal)

    # Order from lower to higher signal values (to plot beautifully)
    idx=order(Signal/Noise)  # order by independent variable in later splines
    # Apply that order to both Signal and Noise
    Signal=Signal[idx]
    Noise=Noise[idx]
    
    imgsave=imgcrop
    imgsave[imgsave<0]=0
    writeTIFF(imgsave, paste0("croppedchart_usedpatches_", NAME, ".tif"), bits.per.sample=16)
    rm(imgsave)
    
    # SNR cuves in dB
    if (image==1) {
        plot(log2(Signal), 20*log10(Signal/Noise), xlim=c(-14,0), ylim=c(-10,20),
             pch=16, cex=0.5, col='blue',
             main='SNR curves - Olympus OM-1',
             xlab='RAW exposure (EV)', ylab='SNR (dB)')
        abline(h=c(0,12), v=seq(-14,0,1), lty=2)
        abline(v=seq(-14,0,1), lty=2, col='gray')
        axis(side=1, at=-14:0)
    } else {
        points(log2(Signal), 20*log10(Signal/Noise), pch=16, cex=0.5, col='blue')        
    }
    
    # # SNR curves in EV
    # plot(log2(Signal), log2(Signal/Noise), xlim=c(-14,0), ylim=c(0,4), col='red',
    #      main=paste0('SNR curves\nOlympus OM-1 at ', NAME),
    #      xlab='RAW exposure (EV)', ylab='SNR (EV)')
    # abline(h=c(0,2), v=seq(-14,0,1), lty=2)


################################

# 5. APPROXIMATION CURVES TO CALCULATE DR VALUES
    
    # Soft cubic splines approximation
    # https://stackoverflow.com/questions/37528590/r-smooth-spline-smoothing-spline-is-not-smooth-but-overfitting-my-data
    # NOTES:
    #   Inverted variables: Signal (DR) = f(Signal/Noise) (threshold)
    #   Splines already in log transformed domains (softer derivatives)
    spline_fit=smooth.spline(20*log10(Signal/Noise), log2(Signal),
                             spar=0.5, nknots=10)  # spar controls smoothness
    lines(predict(spline_fit, 20*log10(Signal/Noise))$y, 20*log10(Signal/Noise),
          col='red', type='l')
    
    # Now calculate the Dynamic Range for a given SNR threshold criteria
    TH_dB=c(12, 0)  # Photographic (12dB) and Engineering (0dB) DR
    DR_EV=c()
    for (threshold in 1:length(TH_dB)) {
        DR_EV=c(DR_EV, -predict(spline_fit, TH_dB[threshold])$y)
        points(-DR_EV[threshold], TH_dB[threshold], pch=3, cex=1.5, col='red')
    }

    if (image==1) {
        DR_df=data.frame(tiff_file=NAME,
                         DR_EV_12dB=DR_EV[1], DR_EV_0dB=DR_EV[2],
                         used_patches=used_patches)        
    } else {
        new_row=data.frame(tiff_file=NAME,
                           DR_EV_12dB=DR_EV[1], DR_EV_0dB=DR_EV[2],
                           used_patches=used_patches)
        DR_df=rbind(DR_df, new_row)
    }
}

dev.off()

# Print calculated DR for each ISO
DR_df=DR_df[order(DR_df$tiff_file), ]
print(DR_df)



