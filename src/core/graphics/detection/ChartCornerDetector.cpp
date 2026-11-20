// File: src/core/graphics/detection/ChartCornerDetector.cpp
/**
 * @file ChartCornerDetector.cpp
 * @brief Implements the chart corner detection algorithm.
 * @details The chart has four bright, round markers near its corners. The detector
 *          looks for round, bright blobs of plausible size (one per image quadrant),
 *          rejecting anything that is not round (patch rows, text, monitor edges,
 *          thin lines, masked-pixel borders) and anything touching the image border.
 *          Several brightness levels are tried so that a brighter spurious object
 *          (reflection, white line, desktop) cannot hide the markers. Among the
 *          candidates, the combination of four that forms the most plausible chart
 *          (convex, large, similar marker sizes and brightness, close to the image
 *          corners) is chosen. If no valid combination exists, the legacy
 *          brightest-pixels method is used as a fallback.
 */
#include "ChartCornerDetector.hpp"
#include "../../math/Math.hpp"
#include <libintl.h>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#define _(string) gettext(string)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace DynaRange::Graphics::Detection {

namespace {

struct BlobCandidate {
    cv::Point2d center;   ///< Intensity-weighted centroid (full-image coordinates).
    double radius = 0.0;  ///< Equivalent radius sqrt(area/pi).
    double brightness = 0.0; ///< Mean value inside the blob.
    double peak = 0.0;       ///< Maximum (smoothed) value inside the blob.
    double roundness = 0.0;  ///< 0..1, 1 = perfect disc.
    int quadrant = -1;    ///< 0=TL, 1=BL, 2=BR, 3=TR (same order as the output).
    double corner_dist = 0.0; ///< Normalised distance to the image corner of its quadrant.
};

/// Quadrant index in output order: TL, BL, BR, TR.
int QuadrantOf(const cv::Point2d& p, int w, int h) {
    const bool left = p.x < w / 2.0;
    const bool top = p.y < h / 2.0;
    if (left && top) return 0;
    if (left && !top) return 1;
    if (!left && !top) return 2;
    return 3;
}

/**
 * @brief Finds round bright blobs in the image at a given absolute threshold.
 */
void CollectRoundBlobs(const cv::Mat& smooth, const cv::Mat& valid_mask, double threshold,
                       double min_r, double max_r, std::vector<BlobCandidate>& out)
{
    cv::Mat bin = smooth > threshold;
    bin &= valid_mask;

    cv::Mat labels, stats, centroids;
    const int n = cv::connectedComponentsWithStats(bin, labels, stats, centroids, 8, CV_32S);
    const int W = smooth.cols, H = smooth.rows;
    const std::array<cv::Point2d, 4> img_corners = {
        cv::Point2d(0, 0), cv::Point2d(0, H), cv::Point2d(W, H), cv::Point2d(W, 0)};
    const double diag = std::sqrt(static_cast<double>(W) * W + static_cast<double>(H) * H);

    for (int i = 1; i < n; ++i) {
        const int area = stats.at<int>(i, cv::CC_STAT_AREA);
        const int bx = stats.at<int>(i, cv::CC_STAT_LEFT);
        const int by = stats.at<int>(i, cv::CC_STAT_TOP);
        const int bw = stats.at<int>(i, cv::CC_STAT_WIDTH);
        const int bh = stats.at<int>(i, cv::CC_STAT_HEIGHT);

        const double r = std::sqrt(area / M_PI);
        if (r < min_r || r > max_r) continue;

        // A disc fills pi/4 (~0.785) of its bounding box and has aspect ~1.
        const double aspect = static_cast<double>(bw) / bh;
        if (aspect < 0.7 || aspect > 1.43) continue;
        const double fill = static_cast<double>(area) / (static_cast<double>(bw) * bh);
        if (fill < 0.65 || fill > 0.90) continue;

        // Roundness from second-order moments: ratio of minor/major axis.
        cv::Rect box(bx, by, bw, bh);
        cv::Mat comp = (labels(box) == i);
        // (computed by hand: cv::moments lives in a different module in OpenCV 4 and 5)
        double m00 = 0, m10 = 0, m01 = 0, m20 = 0, m02 = 0, m11 = 0;
        for (int y = 0; y < bh; ++y) {
            const uchar* mp = comp.ptr<uchar>(y);
            for (int x = 0; x < bw; ++x) {
                if (!mp[x]) continue;
                m00 += 1; m10 += x; m01 += y;
                m20 += static_cast<double>(x) * x; m02 += static_cast<double>(y) * y; m11 += static_cast<double>(x) * y;
            }
        }
        if (m00 <= 0) continue;
        const double cxm = m10 / m00, cym = m01 / m00;
        const double mu20 = m20 / m00 - cxm * cxm;
        const double mu02 = m02 / m00 - cym * cym;
        const double mu11 = m11 / m00 - cxm * cym;
        const double common = std::sqrt(4 * mu11 * mu11 + (mu20 - mu02) * (mu20 - mu02));
        const double l1 = (mu20 + mu02 + common) / 2.0;
        const double l2 = (mu20 + mu02 - common) / 2.0;
        if (l1 <= 0) continue;
        const double axis_ratio = std::sqrt(std::max(l2, 0.0) / l1);
        if (axis_ratio < 0.75) continue;
        // Solidity check: a disc of this area has a known second moment (r^2/4 per axis).
        const double expected_mu = r * r / 4.0;
        const double moment_ratio = (mu20 + mu02) / (2.0 * expected_mu);
        if (moment_ratio < 0.8 || moment_ratio > 1.3) continue;

        // Intensity-weighted centroid above the threshold (sub-pixel, symmetric).
        cv::Mat roi = smooth(box);
        double sw = 0, sx = 0, sy = 0, sum_v = 0, peak = 0;
        for (int y = 0; y < bh; ++y) {
            const uchar* mp = comp.ptr<uchar>(y);
            const float* vp = roi.ptr<float>(y);
            for (int x = 0; x < bw; ++x) {
                if (!mp[x]) continue;
                const double w = vp[x] - threshold;
                sw += w; sx += w * (x + bx); sy += w * (y + by);
                sum_v += vp[x];
                peak = std::max(peak, static_cast<double>(vp[x]));
            }
        }
        if (sw <= 0) continue;

        BlobCandidate c;
        c.center = cv::Point2d(sx / sw, sy / sw);
        c.radius = r;
        c.brightness = sum_v / area;
        c.peak = peak;
        c.roundness = axis_ratio;
        c.quadrant = QuadrantOf(c.center, W, H);
        c.corner_dist = cv::norm(c.center - img_corners[c.quadrant]) / diag;
        out.push_back(c);
    }
}

/// Removes duplicates of the same marker found at several thresholds (keeps the roundest).
std::vector<BlobCandidate> MergeDuplicates(std::vector<BlobCandidate> cands)
{
    std::sort(cands.begin(), cands.end(),
              [](const BlobCandidate& a, const BlobCandidate& b) { return a.roundness > b.roundness; });
    std::vector<BlobCandidate> kept;
    for (const auto& c : cands) {
        bool dup = false;
        for (const auto& k : kept) {
            if (cv::norm(c.center - k.center) < std::max(c.radius, k.radius)) { dup = true; break; }
        }
        if (!dup) kept.push_back(c);
    }
    return kept;
}

double PolygonArea(const std::array<cv::Point2d, 4>& p)
{
    double a = 0;
    for (int i = 0; i < 4; ++i) {
        const auto& p1 = p[i];
        const auto& p2 = p[(i + 1) % 4];
        a += p1.x * p2.y - p2.x * p1.y;
    }
    return std::abs(a) / 2.0;
}

bool IsConvex(const std::array<cv::Point2d, 4>& p)
{
    int sign = 0;
    for (int i = 0; i < 4; ++i) {
        const cv::Point2d d1 = p[(i + 1) % 4] - p[i];
        const cv::Point2d d2 = p[(i + 2) % 4] - p[(i + 1) % 4];
        const double cross = d1.x * d2.y - d1.y * d2.x;
        const int s = (cross > 0) ? 1 : (cross < 0 ? -1 : 0);
        if (s == 0) return false;
        if (sign == 0) sign = s;
        else if (s != sign) return false;
    }
    return true;
}

/**
 * @brief Legacy method: median position of the brightest pixels in each quadrant.
 * @details Kept as a fallback when no set of round markers can be found.
 */
std::optional<std::vector<cv::Point2d>> DetectByBrightestPixels(const cv::Mat& bayer_image, std::ostream& log_stream)
{
    const int DIMX = bayer_image.cols;
    const int DIMY = bayer_image.rows;
    std::vector<cv::Point2d> detected_points;
    std::vector<cv::Rect> sectors = {
        {0, 0, DIMX / 2, DIMY / 2},
        {0, DIMY / 2, DIMX / 2, DIMY / 2},
        {DIMX / 2, DIMY / 2, DIMX / 2, DIMY / 2},
        {DIMX / 2, 0, DIMX / 2, DIMY / 2}
    };
    const double diag = std::sqrt(static_cast<double>(DIMX) * DIMX + static_cast<double>(DIMY) * DIMY);
    const double radius = diag * 0.01;
    const double circle_area = M_PI * radius * radius;
    const double quadrant_area = (DIMX / 2.0) * (DIMY / 2.0);
    const double quantile_fraction = circle_area / quadrant_area;
    const double quantile_threshold = 1.0 - (quantile_fraction / 4.0);
    for (const auto& sector_rect : sectors) {
        cv::Mat quadrant = bayer_image(sector_rect).clone();
        if (quadrant.empty() || quadrant.total() == 0) continue;

        std::vector<double> pixels;
        quadrant.reshape(1, 1).convertTo(pixels, CV_64F);
        double brightness_q_threshold = CalculateQuantile(pixels, quantile_threshold);

        cv::Mat mask;
        cv::threshold(quadrant, mask, brightness_q_threshold, 1.0, cv::THRESH_BINARY);
        mask.convertTo(mask, CV_8U);

        std::vector<cv::Point> bright_pixels;
        cv::findNonZero(mask, bright_pixels);
        if (bright_pixels.empty()) {
            log_stream << _("Warning: No corner circle found in one of the quadrants.") << std::endl;
            return std::nullopt;
        }

        std::vector<int> x_coords, y_coords;
        x_coords.reserve(bright_pixels.size());
        y_coords.reserve(bright_pixels.size());
        for (const auto& p : bright_pixels) {
            x_coords.push_back(p.x);
            y_coords.push_back(p.y);
        }
        std::sort(x_coords.begin(), x_coords.end());
        std::sort(y_coords.begin(), y_coords.end());

        detected_points.emplace_back(x_coords[x_coords.size() / 2] + sector_rect.x,
                                     y_coords[y_coords.size() / 2] + sector_rect.y);
    }
    if (detected_points.size() == 4) return detected_points;
    return std::nullopt;
}

} // namespace

std::optional<std::vector<cv::Point2d>> DetectChartCorners(const cv::Mat& bayer_image, std::ostream& log_stream)
{
    if (bayer_image.empty()) return std::nullopt;

    cv::Mat img;
    bayer_image.convertTo(img, CV_32F);
    const int W = img.cols, H = img.rows;
    const double diag = std::sqrt(static_cast<double>(W) * W + static_cast<double>(H) * H);

    // Light smoothing: removes shot noise and hot pixels, keeps the markers' centres.
    const double sigma = std::max(1.0, diag * 0.001);
    cv::Mat smooth;
    cv::GaussianBlur(img, smooth, cv::Size(0, 0), sigma);

    // Ignore a thin frame around the image: masked/edge pixels, white lines at the
    // border (e.g. Nikon D600) and anything cut by the frame cannot be a marker.
    const int margin = std::max(2, static_cast<int>(std::lround(std::min(W, H) * 0.01)));
    cv::Mat valid_mask = cv::Mat::zeros(H, W, CV_8U);
    if (W > 2 * margin && H > 2 * margin) {
        valid_mask(cv::Rect(margin, margin, W - 2 * margin, H - 2 * margin)).setTo(255);
    }

    // Robust reference level inside the valid area (99.9th percentile).
    std::vector<float> vals;
    vals.reserve(static_cast<size_t>(W) * H / 16 + 1);
    for (int y = margin; y < H - margin; y += 4) {
        const float* p = smooth.ptr<float>(y);
        for (int x = margin; x < W - margin; x += 4) vals.push_back(p[x]);
    }
    if (vals.empty()) return std::nullopt;
    const size_t k_hi = static_cast<size_t>(vals.size() * 0.999);
    const size_t k_med = vals.size() / 2;
    std::nth_element(vals.begin(), vals.begin() + k_hi, vals.end());
    const double hi = vals[k_hi];
    std::nth_element(vals.begin(), vals.begin() + k_med, vals.end());
    const double background = vals[k_med];
    // Noise level of the (smoothed) background, from the lower half of the
    // distribution so that the chart patches do not inflate it.
    std::vector<float> lower(vals.begin(), vals.begin() + k_med);
    for (auto& v : lower) v = static_cast<float>(background - v);
    double noise = 0.0;
    if (!lower.empty()) {
        const size_t k_l = lower.size() / 2;
        std::nth_element(lower.begin(), lower.begin() + k_l, lower.end());
        noise = 1.4826 * lower[k_l];
    }
    // A real marker must stand clearly above the background noise.
    const double MIN_CONTRAST_SIGMAS = 6.0;

    // Markers are typically ~1% of the diagonal in radius; allow a wide range.
    const double min_r = diag * 0.002;
    const double max_r = diag * 0.04;

    std::vector<BlobCandidate> raw_candidates;
    if (hi > background) {
        for (double f : {0.5, 0.35, 0.65, 0.2, 0.8, 0.1}) {
            CollectRoundBlobs(smooth, valid_mask, background + f * (hi - background), min_r, max_r, raw_candidates);
        }
    }
    if (noise > 0) {
        raw_candidates.erase(std::remove_if(raw_candidates.begin(), raw_candidates.end(),
            [&](const BlobCandidate& c) { return (c.peak - background) < MIN_CONTRAST_SIGMAS * noise; }),
            raw_candidates.end());
    }
    std::vector<BlobCandidate> candidates = MergeDuplicates(raw_candidates);

    // Group by quadrant; keep the roundest few per quadrant.
    std::array<std::vector<BlobCandidate>, 4> per_quadrant;
    for (const auto& c : candidates) per_quadrant[c.quadrant].push_back(c);
    const size_t MAX_PER_Q = 10;
    for (auto& q : per_quadrant) {
        std::sort(q.begin(), q.end(),
                  [](const BlobCandidate& a, const BlobCandidate& b) { return a.roundness > b.roundness; });
        if (q.size() > MAX_PER_Q) q.resize(MAX_PER_Q);
    }

    const bool all_quadrants = std::all_of(per_quadrant.begin(), per_quadrant.end(),
                                           [](const auto& q) { return !q.empty(); });
    if (all_quadrants) {
        double best_score = std::numeric_limits<double>::max();
        std::array<const BlobCandidate*, 4> best{};
        const double image_area = static_cast<double>(W) * H;

        for (const auto& a : per_quadrant[0])
        for (const auto& b : per_quadrant[1])
        for (const auto& c : per_quadrant[2])
        for (const auto& d : per_quadrant[3]) {
            const std::array<const BlobCandidate*, 4> set = {&a, &b, &c, &d};
            const std::array<cv::Point2d, 4> pts = {a.center, b.center, c.center, d.center};
            if (!IsConvex(pts)) continue;
            const double area_frac = PolygonArea(pts) / image_area;
            if (area_frac < 0.10) continue;

            double rmin = 1e18, rmax = 0, bmin = 1e18, bmax = 0, dist = 0, round = 0;
            for (const auto* s : set) {
                rmin = std::min(rmin, s->radius); rmax = std::max(rmax, s->radius);
                bmin = std::min(bmin, s->brightness); bmax = std::max(bmax, s->brightness);
                dist += s->corner_dist;
                round += 1.0 - s->roundness;
            }
            const double size_ratio = rmax / rmin;
            if (size_ratio > 1.8) continue;
            const double bright_ratio = (bmin - background > 0) ? (bmax - background) / (bmin - background) : 1e9;

            // Opposite sides of the chart should have similar lengths (mild perspective only).
            const double top = cv::norm(pts[3] - pts[0]), bottom = cv::norm(pts[2] - pts[1]);
            const double left = cv::norm(pts[1] - pts[0]), right = cv::norm(pts[2] - pts[3]);
            if (top <= 0 || bottom <= 0 || left <= 0 || right <= 0) continue;
            const double side_skew = std::abs(std::log(top / bottom)) + std::abs(std::log(left / right));

            // Lower is better: similar sizes/brightness, round, regular quadrilateral,
            // near the corners, large chart.
            // Brightness can legitimately differ between markers (vignetting, monitor
            // uniformity, viewing angle), so it weighs less than geometry.
            const double score = 2.0 * std::log(size_ratio) + 1.0 * std::log(bright_ratio)
                               + 1.0 * round + 1.5 * side_skew + 1.0 * dist - 2.0 * area_frac;
            if (score < best_score) { best_score = score; best = set; }
        }

        if (best[0] != nullptr) {
            return std::vector<cv::Point2d>{best[0]->center, best[1]->center, best[2]->center, best[3]->center};
        }
    }

    log_stream << _("Warning: Could not find the four round corner markers; falling back to brightest-pixel detection.") << std::endl;
    return DetectByBrightestPixels(img, log_stream);
}

} // namespace DynaRange::Graphics::Detection
