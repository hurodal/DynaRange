// Dumps the 4 Bayer planes (RGGB positions 00,01,10,11) of the active area as 16-bit TIFF.
#include <libraw/libraw.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>
int main(int c,char**v){ LibRaw lr; lr.open_file(v[1]); lr.unpack(); auto&S=lr.imgdata.sizes; int pw=S.raw_pitch/2; auto*r=lr.imgdata.rawdata.raw_image;
 printf("pattern %s\n", lr.imgdata.idata.cdesc);
 for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx){ cv::Mat m(S.height/2,S.width/2,CV_16U);
  for(int y=0;y<m.rows;++y)for(int x=0;x<m.cols;++x) m.at<unsigned short>(y,x)=r[(S.top_margin+2*y+dy)*pw+S.left_margin+2*x+dx];
  cv::imwrite(std::string(v[2])+"_p"+std::to_string(dy)+std::to_string(dx)+".tif",m);} }
