// File: src/gui/Constants.hpp
/**
 * @file src/gui/Constants.hpp
 * @brief Centralizes constants related to the Graphical User Interface.
 */
#pragma once

namespace DynaRange::Gui::Constants {

    /**
     * @brief The width in pixels for the generated chart preview thumbnail.
     */
    constexpr int CHART_PREVIEW_WIDTH = 1024;

    /**
     * @brief Scaling factor applied to the base plot dimensions for GUI rendering.
     * @details A factor of 0.75 on a 1920x1080 base results in a 1440x810 image,
     * which provides a good balance of quality and performance for the preview.
     */
    constexpr double GUI_RENDER_SCALE_FACTOR = 0.75;

} // namespace DynaRange::Gui::Constants