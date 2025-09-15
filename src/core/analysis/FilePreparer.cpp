// File: src/core/analysis/FilePreparer.cpp
/**
 * @file src/core/analysis/FilePreparer.cpp
 * @brief Implements file sorting logic based on brightness or EXIF ISO speed.
 */
#include "FilePreparer.hpp"
#include "../io/RawFile.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <vector>
#include <sstream>
#include <opencv2/imgproc.hpp>

namespace fs = std::filesystem;

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