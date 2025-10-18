// File: src/core/analysis/ImageAnalyzer.cpp
/**
 * @file src/core/analysis/ImageAnalyzer.cpp
 * @brief Implements the patch detection and measurement logic for chart analysis.
 */
#include "ImageAnalyzer.hpp"
#include "Constants.hpp"
#include "../../core/DebugConfig.hpp"
#include "../../core/math/estimation/TruncatedNormalEstimator.hpp"
#include <opencv2/imgproc.hpp>
#include <vector>

PatchAnalysisResult AnalyzePatches(cv::Mat imgcrop, int NCOLS, int NROWS, double patch_ratio, bool create_overlay_image, double min_snr_db, double dark_value) {
    cv::Mat image_with_overlays;
    if (create_overlay_image) {
        image_with_overlays = imgcrop.clone();
    }

    const double patch_width_float = static_cast<double>(imgcrop.cols) / NCOLS;
    const double patch_height_float = static_cast<double>(imgcrop.rows) / NROWS;
    const double safe_x = patch_width_float * (1.0 - patch_ratio) / 2.0;
    const double safe_y = patch_height_float * (1.0 - patch_ratio) / 2.0;

    std::vector<double> signal;
    std::vector<double> noise;
    double max_pixel_value = 0.0;
    signal.reserve(NCOLS * NROWS);
    noise.reserve(NCOLS * NROWS);

    for (int j = 0; j < NROWS; j++) {
        for (int i = 0; i < NCOLS; i++) {
            int x1 = round(static_cast<double>(i) * patch_width_float + safe_x);
            int x2 = round(static_cast<double>(i + 1) * patch_width_float - safe_x);
            int y1 = round(static_cast<double>(j) * patch_height_float + safe_y);
            int y2 = round(static_cast<double>(j + 1) * patch_height_float - safe_y);

            if (x1 >= x2 || y1 >= y2) continue;

            cv::Rect roi_rect(x1, y1, x2 - x1, y2 - y1);
            if (roi_rect.x < 0 || roi_rect.y < 0 || roi_rect.x + roi_rect.width > imgcrop.cols || roi_rect.y + roi_rect.height > imgcrop.rows) continue;

            cv::Mat roi = imgcrop(roi_rect);

            cv::Scalar mean_val, stddev_val;
            cv::meanStdDev(roi, mean_val, stddev_val);
            double S = mean_val[0];
            double N = stddev_val[0];

            // --- *** INICIO: NUEVA LÓGICA PARA BLACK = 0 *** ---
            bool potentially_clipped = (dark_value == 0.0);
            int zero_pixel_count = 0;
            double zero_pixel_ratio = 0.0;

            if (potentially_clipped) {
                zero_pixel_count = cv::countNonZero(roi == 0.0);
                zero_pixel_ratio = static_cast<double>(zero_pixel_count) / roi.total();
            }

            // Usar el estimador si black=0 Y hay una proporción significativa de píxeles negros
            // Y la desviación estándar calculada directamente NO es cero (si es cero, es un bloque sólido).
            const double CLIPPING_THRESHOLD_RATIO = 0.01; // Umbral de píxeles negros para activar el estimador (1%)
            if (potentially_clipped && zero_pixel_ratio > CLIPPING_THRESHOLD_RATIO && N > 1e-9)
            {
                std::vector<double> patch_pixels;
                cv::Mat roi_double;
                // Convertir ROI a CV_64F si no lo es ya, necesario para el estimador
                if (roi.type() != CV_64F) {
                    roi.convertTo(roi_double, CV_64F);
                } else {
                    roi_double = roi; // Evitar copia innecesaria si ya es double
                }
                // Copiar los datos del parche a un std::vector<double>
                roi_double.reshape(1, 1).copyTo(patch_pixels);

                // Llamar al nuevo estimador para obtener mu y sigma originales
                auto estimated_params = DynaRange::Math::Estimation::EstimateTruncatedNormal(patch_pixels, 0.0);

                if (estimated_params) {
                    // Si la estimación tuvo éxito, usar los parámetros estimados
                    S = estimated_params->mu;
                    N = estimated_params->sigma;
                    // TODO: (Opcional) Loggear o marcar que este parche usó parámetros estimados
                } else {
                    // Si la estimación falla (ej. datos insuficientes, no convergencia),
                    // descartamos el parche como medida conservadora.
                    // TODO: (Opcional) Loggear un aviso sobre el fallo de estimación
                    continue; // Saltar al siguiente parche
                }
            }
            // --- *** FIN: NUEVA LÓGICA PARA BLACK = 0 *** ---

            // --- Validación de Saturación y SNR Mínimo (Lógica Original) ---
            int sat_count = cv::countNonZero(roi > 0.9);
            double sat_ratio = static_cast<double>(sat_count) / roi.total();

            // Usar los valores S y N (originales o estimados) para la validación final
            if (S > 0 && N > 0 && 20 * log10(S / N) >= min_snr_db && sat_ratio < DynaRange::Analysis::Constants::MAX_SATURATION_RATIO) {
                signal.push_back(S);
                noise.push_back(N);
                max_pixel_value = std::max(max_pixel_value, S); // Usar S (potencialmente estimado)

                // --- Dibujo de Overlays (Lógica Original) ---
                if (create_overlay_image) {
                    #if DYNA_RANGE_DEBUG_MODE == 1 && defined(DYNA_RANGE_DEBUG_PATCH_OUTLINES) // Asumiendo un flag específico
                        cv::rectangle(image_with_overlays, roi_rect.tl() - cv::Point(1,1), roi_rect.br() + cv::Point(1,1),
                                      cv::Scalar(DynaRange::Debug::PATCH_OUTLINE_OUTER_COLOR[2], DynaRange::Debug::PATCH_OUTLINE_OUTER_COLOR[1], DynaRange::Debug::PATCH_OUTLINE_OUTER_COLOR[0]), 1); /*[cite: 41, 148]*/
                        cv::rectangle(image_with_overlays, roi_rect,
                                      cv::Scalar(DynaRange::Debug::PATCH_OUTLINE_INNER_COLOR[2], DynaRange::Debug::PATCH_OUTLINE_INNER_COLOR[1], DynaRange::Debug::PATCH_OUTLINE_INNER_COLOR[0]), 1); /*[cite: 41, 149]*/
                    #else
                        cv::rectangle(image_with_overlays, roi_rect.tl() - cv::Point(1,1), roi_rect.br() + cv::Point(1,1), cv::Scalar(1.0), 1); /*[cite: 150]*/
                        cv::rectangle(image_with_overlays, roi_rect, cv::Scalar(0.0), 1); /*[cite: 151]*/
                    #endif
                }
            }
        }
    }

    PatchAnalysisResult result;
    result.signal = signal;
    result.noise = noise;
    result.max_pixel_value = max_pixel_value;
    if (create_overlay_image) {
        result.image_with_patches = image_with_overlays;
    }

    return result;
}