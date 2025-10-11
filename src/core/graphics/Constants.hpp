// File: src/core/graphics/Constants.hpp
/**
 * @file src/core/graphics/Constants.hpp
 * @brief Centralizes constants and enums related to plotting and graphics generation.
 */
#pragma once

namespace DynaRange::Graphics::Constants {

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
    constexpr PlottingModel PLOTTING_MODEL = PlottingModel::EV_equals_f_SNR;

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

    // --- Centralized Plot Dimension Constants ---
    namespace PlotDefs {
        /// @brief Base reference width for all plot rendering.
        constexpr int BASE_WIDTH = 1920;
        /// @brief Base reference height for all plot rendering.
        constexpr int BASE_HEIGHT = 1080;
    }

} // namespace DynaRange::Graphics::Constants