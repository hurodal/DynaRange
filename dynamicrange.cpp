// dynamicrange.cpp
// Main executable file.
// Orchestrates the program flow: argument parsing, file sorting,
// main processing loop, and saving results.

#include "core/arguments.hpp" // For command-line argument parsing.
#include "core/functions.hpp" // For all data and image processing functions.
#include "spline.h"           // For spline interpolation.

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <filesystem>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <fstream>

#include <libraw/libraw.h>     // For reading RAW files.
#include <opencv2/opencv.hpp>  // For image handling.
#include <Eigen/Dense>         // For linear algebra (keystone correction).

// Headers for Gettext
#include <libintl.h>
#include <locale.h>

// Define the translation macro for abbreviation
#define _(string) gettext(string)

namespace fs = std::filesystem;

// --- MAIN FUNCTION ---
int main(int argc, char* argv[]) {

    // --- GETTEXT INITIALIZATION ---
    setlocale(LC_ALL, ""); // Detects the system language
    bindtextdomain("dynamicrange", "locale"); // Looks for language files in the "locale" folder
    textdomain("dynamicrange"); // Defines the name of the translation file (dynamicrange.mo)

    // --- 0. INITIAL SETUP ---
    // Constants for the color chart analysis.
    const int NCOLS = 11;
    const int NROWS = 7;
    const double SAFE = 50.0;

    // --- 1. ARGUMENT PARSING ---
    // Delegates all command-line logic to the arguments module.
    // The function handles errors and help requests (--help) internally.
    ProgramOptions opts = parse_arguments(argc, argv);

    // Displays the final configuration that will be used for processing.
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n" << _("[FINAL CONFIGURATION]") << "\n";
    std::cout << _("Black level: ") << opts.dark_value << "\n";
    std::cout << _("Saturation point: ") << opts.saturation_value << "\n";
    std::cout << _("Output file: ") << opts.output_filename << "\n\n";

    // --- 2. PRE-ANALYSIS AND SORTING BY EXPOSURE ---
    // To process files in order from lowest to highest exposure without relying on
    // the filename, we first estimate the mean brightness of each file using fast sampling.
    
    // Temporary structure to associate each file with its brightness.
    struct FileExposureInfo {
        std::string filename;
        double mean_brightness;
    };

    std::vector<FileExposureInfo> exposure_data;
    std::cout << _("Pre-analyzing files to sort by exposure (using fast sampling)...") << std::endl;

    // Pre-analysis loop: calculates the estimated brightness of each file.
    for (const std::string& name : opts.input_files) {
        // We use the optimized function that only reads a sample of pixels (e.g., 5%).
        auto mean_val_opt = estimate_mean_brightness(name, 0.05f);

        if (mean_val_opt) {
            exposure_data.push_back({name, *mean_val_opt});
            std::cout << "  - " << _("File: ") << fs::path(name).filename().string()
                      << ", " << _("Estimated brightness: ") << std::fixed << std::setprecision(2) << *mean_val_opt << std::endl;
        }
    }

    // Sorts the list of files based on the mean brightness, from lowest to highest.
    std::sort(exposure_data.begin(), exposure_data.end(),
        [](const FileExposureInfo& a, const FileExposureInfo& b) {
            return a.mean_brightness < b.mean_brightness;
        }
    );

    // Creates the final list of files, now correctly sorted.
    std::vector<std::string> filenames;
    for (const auto& info : exposure_data) {
        filenames.push_back(info.filename);
    }

    if (filenames.empty()){
        std::cerr << _("Error: None of the input files could be processed.") << std::endl;
        return 1;
    }

    std::cout << _("Sorting finished. Starting Dynamic Range calculation process...") << std::endl;
    
    // --- 3. MAIN PROCESSING LOOP ---
    std::vector<DynamicRangeResult> all_results;
    Eigen::VectorXd k; // Vector for keystone correction parameters.

    for (int i = 0; i < filenames.size(); ++i) {
        const std::string& name = filenames[i];
        std::cout << "\n" << _("Processing \"") << name << "\"..." << std::endl;

        // Opening and decoding the RAW file
        LibRaw raw_processor;
        if (raw_processor.open_file(name.c_str()) != LIBRAW_SUCCESS) {
            std::cerr << _("Error: Could not open RAW file: ") << name << std::endl;
            continue;
        }
        if (raw_processor.unpack() != LIBRAW_SUCCESS) {
            std::cerr << _("Error: Could not decode RAW data from: ") << name << std::endl;
            continue;
        }

        const double black_level = opts.dark_value;
        const double sat_level = opts.saturation_value;
        int width = raw_processor.imgdata.sizes.raw_width;
        int height = raw_processor.imgdata.sizes.raw_height;
        std::cout << "  - " << _("Info: Black=") << black_level << ", Saturation=" << sat_level << std::endl;

        // Conversion of RAW data to an OpenCV matrix and normalization
        cv::Mat raw_image(height, width, CV_16U, raw_processor.imgdata.rawdata.raw_image);
        cv::Mat img_float;
        raw_image.convertTo(img_float, CV_32F);
        img_float = (img_float - black_level) / (sat_level - black_level);

        // Extraction of a single channel from the Bayer pattern
        cv::Mat imgBayer(height / 2, width / 2, CV_32FC1);
        for (int r = 0; r < imgBayer.rows; ++r) {
            for (int c = 0; c < imgBayer.cols; ++c) {
                imgBayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2);
            }
        }

        // If it's the first image, calculate the perspective correction parameters.
        if (i == 0) {
            std::vector<cv::Point2d> xu = {{119, 170}, {99, 1687}, {2515, 1679}, {2473, 158}};
            double xtl = (xu[0].x + xu[1].x) / 2.0; double ytl = (xu[0].y + xu[3].y) / 2.0;
            double xbr = (xu[2].x + xu[3].x) / 2.0; double ybr = (xu[1].y + xu[2].y) / 2.0;
            std::vector<cv::Point2d> xd = {{xtl, ytl}, {xtl, ybr}, {xbr, ybr}, {xbr, ytl}};
            k = calculate_keystone_params(xu, xd);
            std::cout << "  - " << _("Keystone parameters calculated.") << std::endl;
        }

        // Apply the correction, crop the image, and analyze the patches
        cv::Mat imgc = undo_keystone(imgBayer, k);
        double xtl = (119.0 + 99.0) / 2.0; double ytl = (170.0 + 158.0) / 2.0;
        double xbr = (2515.0 + 2473.0) / 2.0; double ybr = (1687.0 + 1679.0) / 2.0;
        cv::Rect crop_area(round(xtl), round(ytl), round(xbr - xtl), round(ybr - ytl));
        cv::Mat imgcrop = imgc(crop_area);
        PatchAnalysisResult patch_data = analyze_patches(imgcrop.clone(), NCOLS, NROWS, SAFE);
        
        if (patch_data.signal.empty()) {
            std::cerr << _("Warning: No valid patches found for ") << name << std::endl;
            continue;
        }

        // Sort the patch data by their signal-to-noise ratio (SNR)
        std::vector<double> Signal = patch_data.signal;
        std::vector<double> Noise = patch_data.noise;
        std::vector<size_t> p(Signal.size());
        std::iota(p.begin(), p.end(), 0);
        std::sort(p.begin(), p.end(), [&](size_t i, size_t j){ return (Signal[i]/Noise[i]) < (Signal[j]/Noise[j]); });
        
        std::vector<double> sorted_Signal(Signal.size()), sorted_Noise(Signal.size());
        for(size_t idx = 0; idx < Signal.size(); ++idx) {
            sorted_Signal[idx] = Signal[p[idx]];
            sorted_Noise[idx] = Noise[p[idx]];
        }
        Signal = sorted_Signal;
        Noise = sorted_Noise;

        // Convert signal and noise to the desired units (EV and dB)
        std::vector<double> snr_db, signal_ev;
        for (size_t j = 0; j < Signal.size(); ++j) {
            snr_db.push_back(20 * log10(Signal[j] / Noise[j]));
            signal_ev.push_back(log2(Signal[j]));
        }

        // Fit a spline curve to the data and calculate the DR points
        tk::spline s;
        s.set_points(snr_db, signal_ev);
        double dr_12db = -s(12.0); // DR with "excellent" quality threshold (12 dB)
        double dr_0db = -s(0.0);   // DR with "acceptable limit" threshold (0 dB)
        
        all_results.push_back({name, dr_12db, dr_0db, (int)Signal.size()});
    }

    // --- 4. DISPLAYING AND SAVING RESULTS ---
    std::cout << "\n--- " << _("Dynamic Range Results") << " ---\n";
    std::cout << std::left << std::setw(35) << _("RAW File")
              << std::setw(15) << _("DR (12dB)") << std::setw(15) << _("DR (0dB)")
              << _("Patches") << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    for (const auto& res : all_results) {
        std::cout << std::left << std::setw(35) << fs::path(res.filename).filename().string()
                  << std::fixed << std::setprecision(4) << std::setw(15) << res.dr_12db
                  << std::fixed << std::setprecision(4) << std::setw(15) << res.dr_0db
                  << res.patches_used << std::endl;
    }
    
    // Save the results to the specified CSV file.
    std::ofstream csv_file(opts.output_filename);
    csv_file << "raw_file,DR_EV_12dB,DR_EV_0dB,patches_used\n";
    for (const auto& res : all_results) {
        csv_file << fs::path(res.filename).filename().string() << "," << res.dr_12db << "," << res.dr_0db << "," << res.patches_used << "\n";
    }
    csv_file.close();
    std::cout << "\n" << _("Results saved to ") << opts.output_filename << std::endl;

    return 0;
}