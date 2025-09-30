// File: src/core/DebugConfig.hpp
/**
 * @file src/core/DebugConfig.hpp
 * @brief Centralizes compile-time flags for enabling/disabling debugging features.
 */
#pragma once

// --- Interruptor Maestro de Depuración ---
// Pon a 1 para activar todas las características de depuración de abajo.
// Pon a 0 para compilarlas fuera del código y generar una versión de lanzamiento (release).
#define DYNA_RANGE_DEBUG_MODE 1


#if DYNA_RANGE_DEBUG_MODE == 1
    // --- Banderas de Características de Depuración Individuales ---
    // Activa o desactiva características específicas. El código solo se incluirá
    // si DYNA_RANGE_DEBUG_MODE es 1.

    namespace DynaRange::Debug {
        // Activa el guardado de la imagen de detección de esquinas con cruces y los logs detallados.
        constexpr bool ENABLE_CORNER_DETECTION_DEBUG = true;

        // Define el color de las cruces de depuración en formato BGR (Azul, Verde, Rojo).
        constexpr double CORNER_MARKER_COLOR[] = {0.0, 0.0, 1.0}; // Rojo

        // Define los colores para el borde compuesto de alta visibilidad.
        constexpr double PATCH_OUTLINE_INNER_COLOR[] = {0.0, 0.0, 0.0}; // Negro
        constexpr double PATCH_OUTLINE_OUTER_COLOR[] = {1.0, 1.0, 1.0}; // Blanco (en BGR)
    }

#endif // DYNA_RANGE_DEBUG_MODE