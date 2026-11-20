// Dark-frame statistics for NEF files using LibRaw (same library as DynaRange).
#include <libraw/libraw.h>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>

int main(int argc, char** argv) {
    bool cols = false;
    for (int a = 1; a < argc; ++a) {
        std::string f = argv[a];
        if (f == "--cols") { cols = true; continue; }
        LibRaw lr;
        if (lr.open_file(f.c_str()) != LIBRAW_SUCCESS || lr.unpack() != LIBRAW_SUCCESS) { printf("%s: open fail\n", f.c_str()); continue; }
        auto& S = lr.imgdata.sizes; auto& C = lr.imgdata.color;
        printf("== %s  ISO %.0f  shutter %.6f\n", f.c_str(), lr.imgdata.other.iso_speed, lr.imgdata.other.shutter);
        printf("raw %dx%d  active %dx%d  left_margin %d top_margin %d  pitch %d\n", S.raw_width, S.raw_height, S.width, S.height, S.left_margin, S.top_margin, S.raw_pitch);
        printf("black %u  cblack[0..3] %u %u %u %u  cblack[4,5] %u %u  maximum %u  data_maximum %u  linear_max %.0f %.0f\n",
               C.black, C.cblack[0], C.cblack[1], C.cblack[2], C.cblack[3], C.cblack[4], C.cblack[5], C.maximum, C.data_maximum, C.linear_max[0], C.linear_max[1]);
        const unsigned short* r = lr.imgdata.rawdata.raw_image;
        if (!r) { printf("no bayer raw_image\n"); continue; }
        int pw = S.raw_pitch / 2;
        // Active area stats (as DynaRange GetActiveRawImage)
        double sum = 0, sum2 = 0; long n = 0, zeros = 0; unsigned mn = 65535, mx = 0;
        std::vector<long> hist(65536, 0);
        double cs[4] = {0}, cs2[4] = {0}; long cn[4] = {0}, cz[4] = {0};
        for (int y = S.top_margin; y < S.top_margin + S.height; ++y)
            for (int x = S.left_margin; x < S.left_margin + S.width; ++x) {
                unsigned v = r[y * pw + x];
                sum += v; sum2 += double(v) * v; ++n; hist[v]++;
                if (v == 0) ++zeros;
                mn = std::min(mn, v); mx = std::max(mx, v);
                int c = ((y - S.top_margin) & 1) * 2 + ((x - S.left_margin) & 1);
                cs[c] += v; cs2[c] += double(v) * v; cn[c]++; if (v == 0) cz[c]++;
            }
        double mean = sum / n, sd = std::sqrt(sum2 / n - mean * mean);
        long cum = 0; unsigned med = 0; for (unsigned v = 0; v < 65536; ++v) { cum += hist[v]; if (cum >= n / 2) { med = v; break; } }
        printf("ACTIVE: mean %.4f  sd %.4f  median %u  min %u  max %u  zeros %.2f%%\n", mean, sd, med, mn, mx, 100.0 * zeros / n);
        for (int c = 0; c < 4; ++c) { double m = cs[c] / cn[c]; printf("  CFA%d mean %.4f sd %.4f zeros %.2f%%\n", c, m, std::sqrt(cs2[c] / cn[c] - m * m), 100.0 * cz[c] / cn[c]); }
        printf("  hist 0..15:"); for (int v = 0; v < 16; ++v) printf(" %ld", hist[v]); printf("\n");
        // DynaRange EstimateBlackLevel simulation
        double est = (mn <= 1) ? 256.0 : std::pow(2.0, std::round(std::log2((double)mn)));
        printf("DynaRange auto black (no dark frame) -> %.0f%s\n", est, mn <= 1 ? " (fallback DEFAULT_BLACK_LEVEL)" : "");
        printf("DynaRange black with this file as dark frame (-B) -> %.4f  (truncated-normal path active only if ==0.0)\n", mean);
        printf("DynaRange auto saturation -> %d (2^ceil(log2(maximum))-1)\n", C.maximum ? (1 << (int)std::ceil(std::log2((double)C.maximum))) - 1 : -1);
        // Column profile outside / at edges of active area
        if (cols) {
            printf("COLUMN means (full raw width, rows in active area):\n");
            for (int x = 0; x < S.raw_width; ++x) {
                if (!(x < S.left_margin + 8 || x >= S.left_margin + S.width - 8)) continue;
                double s = 0; for (int y = S.top_margin; y < S.top_margin + S.height; ++y) s += r[y * pw + x];
                printf("  col %d%s mean %.3f\n", x, (x < S.left_margin || x >= S.left_margin + S.width) ? " [margin]" : "", s / S.height);
            }
        }
    }
    return 0;
}
