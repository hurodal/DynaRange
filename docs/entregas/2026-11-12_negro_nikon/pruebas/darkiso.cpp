// Per-ISO dark-frame summary for clipped-black NEFs (LibRaw).
#include <libraw/libraw.h>
#include <cstdio>
#include <cmath>
#include <vector>
#include <numeric>
#include <algorithm>

static double probit(double p) { // inverse normal CDF (Acklam approx, adequate here)
    static const double a[] = {-3.969683028665376e+01, 2.209460984245205e+02, -2.759285104469687e+02, 1.383577518672690e+02, -3.066479806614716e+01, 2.506628277459239e+00};
    static const double b[] = {-5.447609879822406e+01, 1.615858368580409e+02, -1.556989798598866e+02, 6.680131188771972e+01, -1.328068155288572e+01};
    static const double c[] = {-7.784894002430293e-03, -3.223964580411365e-01, -2.400758277161838e+00, -2.549671088433357e+00, 4.374664141464968e+00, 2.938163982698783e+00};
    static const double d[] = {7.784695709041462e-03, 3.224671290700398e-01, 2.445134137142996e+00, 3.754408661907416e+00};
    double q, r;
    if (p < 0.02425) { q = std::sqrt(-2 * std::log(p)); return (((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) / ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1); }
    if (p > 1 - 0.02425) { q = std::sqrt(-2 * std::log(1 - p)); return -(((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) / ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1); }
    q = p - 0.5; r = q * q;
    return (((((a[0]*r+a[1])*r+a[2])*r+a[3])*r+a[4])*r+a[5])*q / (((((b[0]*r+b[1])*r+b[2])*r+b[3])*r+b[4])*r+1);
}

int main(int argc, char** argv) {
    printf("%-13s %6s %5s %6s %8s %8s %7s %5s %9s %9s %8s %8s %6s %8s\n",
           "file", "ISO", "black", "max", "mean", "sd", "zeros%", "step", "mu_fit", "sig_fit", "maskMean", "maskSD", "hot", "DRauto");
    for (int a = 1; a < argc; ++a) {
        LibRaw lr;
        if (lr.open_file(argv[a]) || lr.unpack()) { printf("%s fail\n", argv[a]); continue; }
        auto& S = lr.imgdata.sizes; auto& C = lr.imgdata.color;
        const unsigned short* r = lr.imgdata.rawdata.raw_image; int pw = S.raw_pitch / 2;
        std::vector<long> h(65536, 0); double s = 0, s2 = 0; long n = 0; unsigned mn = 65535;
        for (int y = S.top_margin; y < S.top_margin + S.height; ++y)
            for (int x = S.left_margin; x < S.left_margin + S.width; ++x) { unsigned v = r[y*pw+x]; h[v]++; s += v; s2 += double(v)*v; ++n; mn = std::min(mn, v); }
        double mean = s / n, sd = std::sqrt(s2 / n - mean * mean);
        int step = 0; for (int v = 1; v < 65536; ++v) if (h[v]) step = std::gcd(step, v);
        // censored-normal fit using quantiles above the clip point (p in 0.65..0.95)
        std::vector<double> X, Y; long cum = 0; int v = 0;
        for (double p = 0.65; p <= 0.951; p += 0.05) {
            while (v < 65535 && cum + h[v] < p * n) cum += h[v++];
            if (v > 0) { X.push_back(probit(p)); Y.push_back(v); }
        }
        double mx_ = 0, my = 0; for (size_t i = 0; i < X.size(); ++i) { mx_ += X[i]; my += Y[i]; } mx_ /= X.size(); my /= X.size();
        double sxy = 0, sxx = 0; for (size_t i = 0; i < X.size(); ++i) { sxy += (X[i]-mx_)*(Y[i]-my); sxx += (X[i]-mx_)*(X[i]-mx_); }
        double sig = sxy / sxx, mu = my - sig * mx_;
        double ms = 0, ms2 = 0; long mn2 = 0; long hot = 0;
        for (int y = 0; y < S.height; ++y) for (int x = S.left_margin + S.width; x < S.left_margin + S.width + 8 && x < S.raw_width; ++x) { double q = r[y*pw+x]; ms += q; ms2 += q*q; ++mn2; }
        for (int i = 4000; i < 65536; ++i) hot += h[i];
        double mm = mn2 ? ms / mn2 : 0;
        double auto_black = (mn <= 1) ? 256.0 : std::pow(2.0, std::round(std::log2((double)mn)));
        const char* f = argv[a]; const char* bn = strrchr(f, '/'); bn = bn ? bn + 1 : f;
        printf("%-13s %6.0f %5u %6u %8.3f %8.3f %7.2f %5d %9.2f %9.2f %8.2f %8.3f %6ld %8.0f\n",
               bn, lr.imgdata.other.iso_speed, C.black, C.maximum, mean, sd, 100.0 * h[0] / n, step, mu, sig, mm,
               mn2 ? std::sqrt(ms2 / mn2 - mm * mm) : 0, hot, auto_black);
    }
}
