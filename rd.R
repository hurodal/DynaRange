# RAW Dynamic range measurement
# www.overfitting.net
# https://www.overfitting.net/

library(tiff)


################################

# 1. SAVE RAW DATA ON 16-bit TIFF FILES
# dcraw -v -D -t 0 *.DNG  # replace DNG for any camera RAW format


################################

# 2. READ RAW VALUES FROM DCRAW TIFF FILES AND NORMALIZE

BLACK=255  # Olympus OM-1 black level
SAT=4095  # Olympus OM-1 sat level

filepath=getwd()
filenames=list.files(path=filepath, pattern="\\.tiff$",  # pattern="\\.tif{1,2}$",
                      ignore.case=TRUE, full.names=FALSE)

N=length(filenames)  # number of RAW files to process
# for (image in 1:N) {
for (image in 1:1) {
    NAME=filenames[image]
    cat(paste0('Processing "', NAME, '" ...'))
    
    # Read RAW data
    img=readTIFF(NAME, native=FALSE, convert=FALSE, as.is=TRUE)
    hist(img, breaks=800, main=paste0('"', NAME, '" RAW histogram'))
    abline(v=BLACK, col='red')
    
    # Normalize to 0..1 range (negative values are allowed and preserved)
    img=img-BLACK
    img=img/(SAT-BLACK)
    hist(img, breaks=800)
    abline(v=0, col='red')

    
################################
    
# 3. EXTRACT INDIVIDUAL RAW CHANNEL(S) AND APPLY KEYSTONE CORRECTION ON IT
    
    # Keep G1 channel
    imgG=img[row(img)%%2 & col(img)%%2]
    dim(imgG)=dim(img)/2

    # Distorted points (source)
    xu=c(119, 99, 2515, 2473)  # top-left, bottom-left, bottom-right, top-right
    yu=c(170, 1687, 1679, 158)
    
    # Undistorted points (destination)
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
    
    # Undo distortion function
    undo.keystone = function(xd, yd, k) {
        xu=(k[1]*xd+k[2]*yd+k[3]) / (k[7]*xd+k[8]*yd+1)
        yu=(k[4]*xd+k[5]*yd+k[6]) / (k[7]*xd+k[8]*yd+1)
        return(c(xu, yu))  # return pair (xu, yu)
    }
    
    # Check
    for (i in 1:4) print(undo.keystone(xd[i], yd[i], k))
    
    # Plot trapezoids
    png("keystonecorrection.png", width=800, height=600)
        plot(c(xd, xd[1]), c(yd, yd[1]), type='l', col='red', asp=1,
             xlab='X', ylab='Y', xlim=c(1, ncol(imgG)), ylim=c(nrow(imgG), 1))
        lines(c(xu, xu[1]), c(yu, yu[1]), type='l', col='blue')
        for (i in 1:4) {
            lines(c(xd[i], xu[i]), c(yd[i], yu[i]), type='l', lty=3, col='darkgray')
        }
        abline(h=c(1,nrow(imgG)), v=c(1,ncol(imgG)))
    dev.off()
    
    # Correct keystone distortion
    DIMX=ncol(imgG)
    DIMY=nrow(imgG)
    
    imgc=imgG*0
    for (x in 1:DIMX) {
        for (y in 1:DIMY) {
            xuyu=round(undo.keystone(x, y, k))
            if (xuyu[1]>=1 & xuyu[1]<=DIMX & xuyu[2]>=1 & xuyu[2]<=DIMY)
                imgc[y, x]=imgG[xuyu[2], xuyu[1]]  # nearest neighbour interp
        }
    }
    imgsave=imgc
    imgsave[imgsave<0]=0
    writeTIFF(imgsave, paste0("corrected_", NAME, ".tif"), bits.per.sample=16)
    
    
################################
    
# 4. READ PATCHES TO FORM 7x11 GRID AND COLLECT (EV,SNR) PAIRS
    
    # Crop patches area
    imgcrop=imgc[round(ytl):round(ybr), round(xtl):round(xbr)]
    DIMX=ncol(imgcrop)
    DIMY=nrow(imgcrop)
    
    NCOLS=11
    NROWS=7
    SAFE=50
    Signal=vector(mode="numeric", length=NCOLS*NROWS)
    Noise=Signal
    
    k=1
    for (i in 1:NCOLS) {
        for (j in 1:NROWS) {
            x1=round((i-1)*DIMX/NCOLS + SAFE)
            x2=round( i   *DIMX/NCOLS - SAFE)
            y1=round((j-1)*DIMY/NROWS + SAFE)
            y2=round( j   *DIMY/NROWS - SAFE)
            patch=which(row(imgcrop)>=y1 & row(imgcrop)<=y2 & 
                        col(imgcrop)>=x1 & col(imgcrop)<=x2)
            values=imgcrop[patch]
            Signal[k]=mean(values)  # S=mean
            Noise[k]=var(values)^0.5  # N=stdev
            k=k+1
            
            imgcrop[y1:y2,x1]=0
            imgcrop[y1:y2,x2]=0
            imgcrop[y1,x1:x2]=0
            imgcrop[y2,x1:x2]=0
        }
    }
    # Order from lower to higher signal values (to plot beautifully)
    idx=order(Signal)
    # Apply that order to both Signal and Noise
    Signal=Signal[idx]
    Noise=Noise[idx]
    
    imgsave=imgcrop
    imgsave[imgsave<0]=0
    writeTIFF(imgsave, paste0("cropwithpatches_", NAME, ".tif"), bits.per.sample=16)
    
    # SNR cuves in dB
    png(paste0("snr_", NAME, ".png"), width=1280, height=800)
        plot(log2(Signal), 20*log10(Signal/Noise), xlim=c(-12,0), ylim=c(-5,20), col='red',
             main=paste0('SNR curves\nOlympus OM-1 at ', NAME),
             xlab='RAW exposure (EV)', ylab='SNR (dB)')
        abline(h=c(0,12), v=seq(-12,0,1), lty=2, col='gray')
        axis(side=1, at=-12:0)

        
# SOLUCIONAR 2 PROBLEMAS DE LAS TOMAS DE ISOS MUY ALTOS:
# 1. LOS PARCHES MÁS OSCUROS PUEDEN TENER MEDIA <0
# 2. LOS PARCHES MÁS EXPUESTOS TIENEN VALORES RAW SATURADOS
# CUALQUIER PARCHE DONDE OCURRA UNA DE LAS 2 COSAS DEBE DESECHARSE
# -> USAR Signal=c() en lugar de una long. predefinida
        


################################

# 5. APPROXIMATION CURVES TO CALCULATE DR VALUES
    
    # Soft cubic splines approximation
    # spline_fit=smooth.spline(Signal, Signal/Noise, spar=0.5)  # spar controls smoothness
    # points(log2(Signal), 20*log10(predict(spline_fit, Signal)$y), col='blue', type='l')
    
    # But we'll use the inverse spline: Signal/Noise (threshold) -> Signal (DR)
        spline_fit=smooth.spline(Signal/Noise, Signal, spar=0.8)  # spar controls smoothness
        points(log2(predict(spline_fit, Signal/Noise)$y), 20*log10(Signal/Noise), col='blue', type='l')
    dev.off()
    
    # Now calculate the Dynamic Range for a given SNR threshold criteria
    TH=c(12, 0)  # Photographic (12dB) and Engineering (0dB) DR
    
    for (criteria in 1:length(TH)) {
        DR=predict(spline_fit, 10^(TH[criteria]/20))$y
        print(paste0("Dynamic range for ", TH[criteria], "dB: ",
                     round(-log2(DR),1), "EV"))
    }
}


# SNR curves in EV
plot(log2(Signal), log2(Signal/Noise), xlim=c(-12,0), ylim=c(0,5), col='red',
     main=paste0('SNR curves\nOlympus OM-1 at ', NAME),
     xlab='RAW exposure (EV)', ylab='SNR (EV)')
abline(h=2, lty=2)




