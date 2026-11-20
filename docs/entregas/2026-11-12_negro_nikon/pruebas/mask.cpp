#include <libraw/libraw.h>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
int main(int argc,char**argv){ for(int a=1;a<argc;++a){ LibRaw lr; lr.open_file(argv[a]); lr.unpack(); auto&S=lr.imgdata.sizes; const unsigned short*r=lr.imgdata.rawdata.raw_image; int pw=S.raw_pitch/2;
 double s=0,s2=0; long n=0, hot=0; for(int y=0;y<S.height;++y) for(int x=6034;x<=6041;++x){double v=r[y*pw+x]; s+=v;s2+=v*v;++n;}
 double m=s/n; printf("%s masked cols 6034-6041: mean %.3f sd %.3f\n",argv[a],m,std::sqrt(s2/n-m*m));
 // truncated-normal (mu=0 assumed) sigma estimates from active area, excluding hot pixels >2000
 double sp=0; long np=0,nz=0,nt=0; std::vector<unsigned short> v; for(int y=0;y<S.height;++y) for(int x=0;x<S.width;++x){unsigned short p=r[y*pw+x]; if(p>2000){hot++;continue;} nt++; if(p==0) nz++; sp+= (double)p*p;}
 printf("  active: hot(>2000) %ld  zeros %.2f%%  sigma(half-normal RMS, mu=0) %.2f\n",hot,100.0*nz/nt,std::sqrt(2*sp/nt));
}}
