// File: src/core/Constants.hpp
/**
 * @file src/core/Constants.hpp
 * @brief Centralizes global constants and definitions not tied to argument defaults.
 */
#pragma once

namespace DynaRange::Constants {

    // Command-line executable name.
    constexpr const char* CLI_EXECUTABLE_NAME = "rango";
    /**
     * @brief The minimum percentage of the total image area that the detected
     * chart must occupy to be considered valid.
     */
    constexpr double MINIMUM_CHART_AREA_PERCENTAGE = 0.50; // 50%

    /**
     * @enum PlottingModel
     * @brief Defines the mathematical model used for generating plot curve points.
     */
    enum class PlottingModel {
        SNR_equals_f_EV, ///< C++ Model: SNR_dB = f(EV)
        EV_equals_f_SNR  ///< R Model (Future): EV = f(SNR_dB)
    };
    /**
     * @brief The model to be used for generating the SNR curve for plotting.
     */
    constexpr PlottingModel PLOTTING_MODEL = PlottingModel::SNR_equals_f_EV;

    /**
     * @enum PlotOutputFormat
     * @brief Defines the output format for generated plots.
     */
    enum class PlotOutputFormat { PNG, PDF, SVG };

    /**
     * @brief Scaling factor for vector output formats (PDF, SVG).
     * @details A factor of 2.0 will create a document twice as large (e.g., 3840x2160).
     */
    constexpr double VECTOR_PLOT_SCALE_FACTOR = 2.0;

    /**
     * @brief Minimum Signal-to-Noise Ratio (in dB) for a patch to be considered valid.
     * @details Aligned with the R script's reference value to include deep shadow data.
     */
    constexpr double MIN_SNR_DB_THRESHOLD = -90.0;

    /**
     * @brief Maximum ratio of saturated pixels for a patch to be considered valid.
     * @details A value of 0.01 means the patch is discarded if 1% or more of its pixels are saturated.
     */
    constexpr double MAX_SATURATION_RATIO = 0.01;

} // namespace DynaRange::Constants