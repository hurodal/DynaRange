// Synthetic check: Gaussian noise, integer quantisation, clip at 0, optional digital gain.
#include "TruncatedNormalEstimator.hpp"
#include <cstdio>
#include <cmath>
#include <random>
#include <vector>
using namespace DynaRange::Math::Estimation;
int main(){
  std::mt19937 rng(1);
  printf("%6s %5s %6s | %8s %8s | %8s %8s | %8s %8s\n","sigma","gain","mu","plain_mu","plain_sd","new_mu","new_sig","err_mu","err_sig%");
  double sigmas[]={0.6,1.3,3,5.4,9.4,10,10,20};
  int gains[]={1,1,1,1,1,2,4,4};
  double mus[]={0,0,0,0,0,0,0,30};
  for(int t=0;t<8;++t){ std::normal_distribution<double> N(mus[t],sigmas[t]);
    std::vector<double> d; for(int i=0;i<40000;++i){ double a=std::round(N(rng)); if(a<0)a=0; d.push_back(a*gains[t]/16383.0);}
    double m=0,m2=0; for(double v:d){m+=v;m2+=v*v;} m/=d.size(); double sd=std::sqrt(m2/d.size()-m*m);
    double mp=0; for(double v:d) if(v>0&&(mp==0||v<mp)) mp=v;
    auto e=EstimateTruncatedNormal(d,0.5*mp); double tm=mus[t]*gains[t], ts=sigmas[t]*gains[t];
    printf("%6.1f %5d %6.0f | %8.3f %8.3f | %8.3f %8.3f | %8.3f %8.1f\n",ts,gains[t],tm,m*16383,sd*16383,e->mu*16383,e->sigma*16383,e->mu*16383-tm,100*(e->sigma*16383/ts-1));
  }}
