/**
 * @file core/Analysis.cpp
 * @brief Implements high-level analysis functions like patch analysis and frame processing.
 */
#include "Analysis.hpp"
#include "RawFile.hpp"
#include "Math.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

namespace { // Anonymous namespace for internal helpers

/**
 * @brief Calculates SNR and EV values, applies normalization, and fits a polynomial curve.
 * @param patch_data The result of the patch analysis.
 * @param opts The program options, used for normalization settings and polynomial order.
 * @param camera_resolution_mpx The camera's actual resolution in megapixels.
 * @return An SnrCurve struct containing the calculated curve data.
 */
SnrCurve CalculateSnrCurve(const PatchAnalysisResult& patch_data, const ProgramOptions& opts, double camera_resolution_mpx) {
    SnrCurve curve;
    
    // 1. Calculate linear SNR values first
    std::vector<double> snr_linear;
    snr_linear.reserve(patch_data.signal.size());
    for (size_t i = 0; i < patch_data.signal.size(); ++i) {
        if (patch_data.noise[i] > 0) {
            snr_linear.push_back(patch_data.signal[i] / patch_data.noise[i]);
        }
    }

    // 2. Apply normalization factor if requested by the user
    if (opts.dr_normalization_mpx > 0 && camera_resolution_mpx > 0) {
        double norm_factor = sqrt(camera_resolution_mpx / opts.dr_normalization_mpx);
        for (double& snr : snr_linear) {
            snr *= norm_factor;
        }
    }

    // 3. Convert final linear SNR to dB and calculate EV
    for (size_t i = 0; i < snr_linear.size(); ++i) {
        curve.snr_db.push_back(20 * log10(snr_linear[i]));
        curve.signal_ev.push_back(log2(patch_data.signal[i]));
    }

    // 4. Fit polynomial to the final data
    cv::Mat signal_mat_global(curve.signal_ev.size(), 1, CV_64F, curve.signal_ev.data());
    cv::Mat snr_mat_global(curve.snr_db.size(), 1, CV_64F, curve.snr_db.data());
    
    PolyFit(signal_mat_global, snr_mat_global, curve.poly_coeffs, opts.poly_order);
    return curve;
}


/**
 * @brief Calculates the dynamic range values for a set of thresholds.
 * @param snr_curve The calculated SNR curve.
 * @param thresholds_db The vector of SNR thresholds in dB.
 * @return A map of threshold to calculated dynamic range in EV.
 */
std::map<double, double> CalculateDynamicRange(const SnrCurve& snr_curve, const std::vector<double>& thresholds_db) {
    std::map<double, double> dr_values_ev;
    if (snr_curve.signal_ev.empty()) {
        return dr_values_ev;
    }

    auto min_max_ev = std::minmax_element(snr_curve.signal_ev.begin(), snr_curve.signal_ev.end());

    for (const double threshold_db : thresholds_db) {
        auto ev_opt = FindIntersectionEV(snr_curve.poly_coeffs, threshold_db, *min_max_ev.first, *min_max_ev.second);
        if (ev_opt) {
            dr_values_ev[threshold_db] = -(*ev_opt);
        }
    }
    return dr_values_ev;
}
} // end anonymous namespace

std::pair<DynamicRangeResult, CurveData> CalculateResultsFromPatches(
    const PatchAnalysisResult& patch_data, 
    const ProgramOptions& opts, 
    const std::string& filename,
    double camera_resolution_mpx) 
{
    // Pass all necessary info to the curve calculation function
    SnrCurve snr_curve = CalculateSnrCurve(patch_data, opts, camera_resolution_mpx);
    
    DynamicRangeResult dr_result;
    dr_result.filename = filename;
    dr_result.patches_used = (int)patch_data.signal.size();
    dr_result.dr_values_ev = CalculateDynamicRange(snr_curve, opts.snr_thresholds_db);
    
    CurveData curve_data = {
        filename, 
        "", // plot_label (se rellenará en Processing.cpp)
        "", // camera_model (se rellenará en Processing.cpp)
        snr_curve.signal_ev, 
        snr_curve.snr_db, 
        snr_curve.poly_coeffs.clone(), 
        opts.generated_command
    };

    return {dr_result, curve_data};
}

PatchAnalysisResult AnalyzePatches(cv::Mat imgcrop, int NCOLS, int NROWS, double patch_ratio) {
    std::vector<double> signal_vec, noise_vec;
    const double patch_width = (double)imgcrop.cols / NCOLS;
    const double patch_height = (double)imgcrop.rows / NROWS;
    const double safe_x = patch_width * (1.0 - patch_ratio) / 2.0;
    const double safe_y = patch_height * (1.0 - patch_ratio) / 2.0;

    for (int j = 0; j < NROWS; ++j) {
        for (int i = 0; i < NCOLS; ++i) {
            int x1 = round((double)i * patch_width + safe_x);
            int x2 = round((double)(i + 1) * patch_width - safe_x);
            int y1 = round((double)j * patch_height + safe_y);
            int y2 = round((double)(j + 1) * patch_height - safe_y);
            
            if (x1 >= x2 || y1 >= y2) continue;
            
            cv::Mat patch = imgcrop(cv::Rect(x1, y1, x2 - x1, y2 - y1));
            cv::Scalar mean, stddev;
            cv::meanStdDev(patch, mean, stddev);
            double S = mean[0], N = stddev[0];
            int sat_count = cv::countNonZero(patch > 0.9);
            double sat_ratio = (double)sat_count / (patch.rows * patch.cols);
            
            if (S > 0 && N > 0 && 20 * log10(S / N) >= -10 && sat_ratio < 0.01) {
                signal_vec.push_back(S); 
                noise_vec.push_back(N);
                cv::rectangle(imgcrop, {x1, y1}, {x2, y2}, cv::Scalar(0.0), 1);
                cv::rectangle(imgcrop, {x1 - 1, y1 - 1}, {x2 + 1, y2 + 1}, cv::Scalar(1.0), 1);
            }
        }
    }
    return {signal_vec, noise_vec, imgcrop};
}

std::optional<double> ProcessDarkFrame(const std::string& filename, std::ostream& log_stream) {
    log_stream << "[INFO] Calculating black level from: " << filename << "..." << std::endl;
    RawFile dark_file(filename);
    if (!dark_file.Load()) return std::nullopt;

    cv::Mat raw_img = dark_file.GetRawImage();
    if (raw_img.empty()) return std::nullopt;

    // Convert cv::Mat to vector for statistical analysis
    std::vector<double> pixels;
    pixels.reserve(raw_img.total());
    raw_img.reshape(1, 1).convertTo(pixels, CV_64F);

    double mean_value = CalculateMean(pixels);
    log_stream << "[INFO] -> Black level obtained: " << std::fixed << std::setprecision(2) << mean_value << std::endl;
    return mean_value;
}

std::optional<double> ProcessSaturationFrame(const std::string& filename, std::ostream& log_stream) {
    log_stream << "[INFO] Calculating saturation point from: " << filename << "..." << std::endl;
    RawFile sat_file(filename);
    if (!sat_file.Load()) return std::nullopt;
    
    cv::Mat raw_img = sat_file.GetRawImage();
    if (raw_img.empty()) return std::nullopt;

    std::vector<double> pixels;
    pixels.reserve(raw_img.total());
    raw_img.reshape(1, 1).convertTo(pixels, CV_64F);

    // Using 5th percentile of the brightest pixels to avoid sensor defects
    double quantile_value = CalculateQuantile(pixels, 0.05);
    log_stream << "[INFO] -> Saturation point obtained (5th percentile): " << std::fixed << std::setprecision(2) << quantile_value << std::endl;
    return quantile_value;
}

bool PrepareAndSortFiles(ProgramOptions& opts, std::ostream& log_stream) {
    // A simple constant to easily switch the default sorting method
    constexpr bool USE_EXIF_SORT_DEFAULT = false;

    struct FileInfo {
        std::string filename;
        double mean_brightness = 0.0;
        float iso_speed = 0.0f;
    };

    std::vector<FileInfo> file_info_list;
    bool exif_sort_possible = true;

    log_stream << "Pre-analyzing files to determine sorting order..." << std::endl;
    
    for (const std::string& name : opts.input_files) {
        RawFile raw_file(name);
        if (!raw_file.Load()) continue;

        FileInfo info;
        info.filename = name;

        // Method A: Brightness sampling
        cv::Mat raw_img = raw_file.GetRawImage();
        if (!raw_img.empty()) {
            info.mean_brightness = cv::mean(raw_img)[0];
        }

        // Method B: EXIF ISO speed
        info.iso_speed = raw_file.GetIsoSpeed();
        if (info.iso_speed <= 0) {
            exif_sort_possible = false; // Mark EXIF sort as impossible if any file lacks ISO data
        }

        file_info_list.push_back(info);
        log_stream << "  - File: " << fs::path(name).filename().string() 
                   << ", Brightness: " << std::fixed << std::setprecision(2) << info.mean_brightness
                   << ", ISO: " << info.iso_speed << std::endl;
    }

    if (file_info_list.empty()) {
        log_stream << "Error: None of the input files could be processed." << std::endl;
        return false;
    }

    // LIST A: Sort by mean brightness
    std::vector<FileInfo> list_a = file_info_list;
    std::sort(list_a.begin(), list_a.end(), [](const FileInfo& a, const FileInfo& b) {
        return a.mean_brightness < b.mean_brightness;
    });

    // LIST B: Sort by ISO speed (if possible)
    std::vector<FileInfo> list_b;
    if (exif_sort_possible) {
        list_b = file_info_list;
        std::sort(list_b.begin(), list_b.end(), [](const FileInfo& a, const FileInfo& b) {
            return a.iso_speed < b.iso_speed;
        });

        // Compare the two lists
        bool lists_match = std::equal(list_a.begin(), list_a.end(), list_b.begin(), 
                                      [](const FileInfo& a, const FileInfo& b){ return a.filename == b.filename; });
        if (lists_match) {
            log_stream << "\n[INFO] Sorting by brightness and by ISO produce the same file order." << std::endl;
        } else {
            log_stream << "\n[WARNING] Sorting by brightness and by ISO produce DIFFERENT file orders." << std::endl;
        }
    } else {
        log_stream << "\n[WARNING] Cannot use EXIF data. ISO not available in all files. Using brightness sorting." << std::endl;
    }

    // --- SELECCIÓN DE ORDENACIÓN ---
    const std::vector<FileInfo>* final_sorted_list = &list_a; // Default to brightness sort
    if (USE_EXIF_SORT_DEFAULT && exif_sort_possible) {
        final_sorted_list = &list_b;
        log_stream << "[INFO] Using final file order from: EXIF ISO (List B)" << std::endl;
    } else {
        log_stream << "[INFO] Using final file order from: Image Brightness (List A)" << std::endl;
    }
    
    // --- LÓGICA DE ETIQUETADO (CORREGIDA) ---
    // La elección de la etiqueta ahora es independiente de la ordenación.
    // Se basa únicamente en si los datos EXIF están disponibles para TODOS los ficheros.
    opts.input_files.clear();
    opts.plot_labels.clear();
    for (const auto& info : *final_sorted_list) {
        opts.input_files.push_back(info.filename);
        
        if (exif_sort_possible) {
            // Si es posible usar EXIF, las etiquetas SIEMPRE serán el ISO.
            std::stringstream label_ss;
            label_ss << "ISO " << static_cast<int>(info.iso_speed);
            opts.plot_labels[info.filename] = label_ss.str();
        } else {
            // Si EXIF falla para CUALQUIER fichero, se usa el nombre de fichero para TODOS.
            opts.plot_labels[info.filename] = fs::path(info.filename).stem().string();
        }
    }

    log_stream << "Sorting finished. Starting Dynamic Range calculation process..." << std::endl;
    return true;
}