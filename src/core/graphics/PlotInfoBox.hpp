// File: src/core/graphics/PlotInfoBox.hpp
/**
 * @file PlotInfoBox.hpp
 * @brief Declares a class to manage and draw an informational box on a plot.
 * @details This module encapsulates the data and rendering logic for a key-value
 * info box, typically displayed in a corner of a graph.
 */
#pragma once
#include <cairo/cairo.h>
#include <string>
#include <vector>
#include <utility>

class PlotInfoBox {
public:
    /**
     * @brief Adds a new key-value pair, with an optional annotation, to the info box.
     * @param label The label or key (e.g., "Black").
     * @param value The value to display (e.g., "256.00").
     * @param annotation An optional string to append in a different color (e.g., "(estimated)").
     */
    void AddItem(const std::string& label, const std::string& value, const std::string& annotation = "");

    /**
     * @brief Draws all the added items onto the cairo context.
     * @param cr The cairo drawing context.
     */
    void Draw(cairo_t* cr) const;

private:
    // This struct now holds the data for each item, including the optional annotation.
    struct InfoItem {
        std::string label;
        std::string value;
        std::string annotation;
    };
    std::vector<InfoItem> m_items;
};