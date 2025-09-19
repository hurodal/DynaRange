// File: src/core/setup/SensorResolution.cpp
/**
 * @file SensorResolution.cpp
 * @brief Implements the sensor resolution detection logic.
 */
#include "SensorResolution.hpp"
#include "../io/RawFile.hpp"
#include <iomanip>

double DetectSensorResolution(const std::vector<std::string>& input_files, std::ostream& log_stream) {
    for (const std::string& name : input_files) {
        RawFile raw_file(name);
        if (!raw_file.Load()) {
            continue;
        }

        // Attempt to get resolution from specific metadata first
        double sensor_res_from_metadata = raw_file.GetSensorResolutionMPx();
        if (sensor_res_from_metadata > 0.0) {
            log_stream << "[INFO] Sensor resolution detected from RAW metadata: "
                       << std::fixed << std::setprecision(1) << sensor_res_from_metadata << " Mpx\n";
            return sensor_res_from_metadata;
        }
        
        // Fallback to image dimensions if metadata tag is missing
        int width = raw_file.GetWidth();
        int height = raw_file.GetHeight();
        if (width > 0 && height > 0) {
            double sensor_res_from_dims = static_cast<double>(width * height) / 1000000.0;
            if (sensor_res_from_dims > 0.1) { // Avoid absurdly small values
                log_stream << "[INFO] Sensor resolution inferred from RAW dimensions: "
                           << std::fixed << std::setprecision(1) << sensor_res_from_dims << " Mpx\n";
                return sensor_res_from_dims;
            }
        }
    }
    return 0.0; // Return 0.0 if no resolution could be determined
}