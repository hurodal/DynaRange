// core/engine.cpp
#include "engine.hpp"
#include "functions.hpp"
#include "spline.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <filesystem>
#include <numeric>
#include <algorithm>
#include <fstream>
#include <libraw/libraw.h>
#include <opencv2/opencv.hpp>
#include <Eigen/Dense>

namespace fs = std::filesystem;

bool run_dynamic_range_analysis(const ProgramOptions& opts, std::ostream& log_stream) {
    const int NCOLS = 11;
    const int NROWS = 7;
    const double SAFE = 50.0;

    std::vector<DynamicRangeResult> all_results;
    Eigen::VectorXd k;
    
    const auto& filenames = opts.input_files;

    for (int i = 0; i < filenames.size(); ++i) {
        const std::string& name = filenames[i];
        log_stream << "\nProcessing \"" << name << "\"..." << std::endl;

        LibRaw raw_processor;
        if (raw_processor.open_file(name.c_str()) != LIBRAW_SUCCESS) {
            log_stream << "Error: Could not open RAW file: " << name << std::endl;
            return false;
        }
        if (raw_processor.unpack() != LIBRAW_SUCCESS) {
            log_stream << "Error: Could not decode RAW data from: " << name << std::endl;
            return false;
        }
        // ... (resto del bucle principal, usando log_stream en vez de std::cout/cerr y sin _() )
    }

    // ... (resto de la lógica de guardado de resultados, usando log_stream)
    
    std::ofstream csv_file(opts.output_filename);
    csv_file << "raw_file,DR_EV_12dB,DR_EV_0dB,patches_used\n";
    for (const auto& res : all_results) {
        csv_file << fs::path(res.filename).filename().string() << "," << res.dr_12db << "," << res.dr_0db << "," << res.patches_used << "\n";
    }
    csv_file.close();
    log_stream << "\nResults saved to " << opts.output_filename << std::endl;

    return true;
}