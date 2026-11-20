// Validates DynaRange's censored-normal estimator on Nikon D610 dark frames.
// Truth: black is subtracted in-camera, so mu ~ 0; sigma ~ quantile-fit value.
#include <libraw/libraw.h>
#include "TruncatedNormalEstimator.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <optional>
#include <algorithm>

namespace DynaRange::Math::Estimation::Internal {
std::optional<NormalParameters> EstimateWithGradientDescent(const std::vector<double>&, double, double, double);
double CalculateStdDev(const std::vector<double>& data, double mean);
}
using namespace DynaRange::Math::Estimation;

static std::optional<NormalParameters> OldPath(const std::vector<double>& d) {
    // Replicates the previous dispatcher: init from values above 0, gradient descent, c = 0.
    std::vector<double> above; for (double v : d) if (v > 0) above.push_back(v);
    double m = 0; for (double v : above) m += v; m /= above.size();
    double s = Internal::CalculateStdDev(above, m);
    return Internal::EstimateWithGradientDescent(d, 0.0, m, s);
}

int main(int argc, char** argv) {
    const double SAT = 16383.0;
    printf("%-13s %6s | %9s %9s | %9s %9s | %9s %9s\n", "file", "ISO", "plain_mu", "plain_sd", "old_mu", "old_sig", "new_mu", "new_sig");
    for (int a = 1; a < argc; ++a) {
        LibRaw lr; if (lr.open_file(argv[a]) || lr.unpack()) continue;
        auto& S = lr.imgdata.sizes; const unsigned short* r = lr.imgdata.rawdata.raw_image; int pw = S.raw_pitch / 2;
        // R channel (RGGB -> even row, even col), central 200x200 block of the Bayer sub-image
        std::vector<double> d; int cy = S.height / 4, cx = S.width / 4;
        for (int y = cy - 100; y < cy + 100; ++y) for (int x = cx - 100; x < cx + 100; ++x) {
            double v = r[(2 * y) * pw + 2 * x];
            if (v > 2000) continue; // hot pixel
            d.push_back(v / SAT);
        }
        double m = 0, m2 = 0; for (double v : d) { m += v; m2 += v * v; } m /= d.size(); double sd = std::sqrt(m2 / d.size() - m * m);
        double minpos = 0; for (double v : d) if (v > 0 && (minpos == 0 || v < minpos)) minpos = v;
        auto o = OldPath(d);
        auto n = EstimateTruncatedNormal(d, 0.5 * minpos);
        const char* bn = strrchr(argv[a], '/'); bn = bn ? bn + 1 : argv[a];
        printf("%-13s %6.0f | %9.3f %9.3f | %9.3f %9.3f | %9.3f %9.3f\n", bn, lr.imgdata.other.iso_speed, m * SAT, sd * SAT,
               o ? o->mu * SAT : NAN, o ? o->sigma * SAT : NAN, n ? n->mu * SAT : NAN, n ? n->sigma * SAT : NAN);
    }
}
