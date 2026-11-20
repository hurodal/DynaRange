#include <libraw/libraw.h>
#include <cstdio>
#include <cstring>
#include <vector>
int main(int c,char**v){for(int a=1;a<c;++a){LibRaw lr;lr.open_file(v[a]);lr.unpack();auto&S=lr.imgdata.sizes;auto*r=lr.imgdata.rawdata.raw_image;int pw=S.raw_pitch/2;std::vector<long>h(65536);
for(int y=0;y<S.height;++y)for(int x=0;x<S.width;++x)h[r[y*pw+x]]++;
int occ=0;for(int i=1;i<=400;++i)if(h[i]>50)occ++;
printf("%s ISO %.0f  codes used in 1..400 (count>50): %d  first nonzero codes:",strrchr(v[a],'/')?strrchr(v[a],'/')+1:v[a],lr.imgdata.other.iso_speed,occ);
int k=0;for(int i=1;i<400&&k<10;++i)if(h[i]>50){printf(" %d",i);k++;}printf("\n");}}
