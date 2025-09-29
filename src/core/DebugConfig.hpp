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
        // Activa el guardado de la imagen de detección de esquinas y los logs detallados.
        constexpr bool ENABLE_CORNER_DETECTION_DEBUG = true;

        // Aquí se pueden añadir otras banderas de depuración en el futuro...
        // constexpr bool ENABLE_ANOTHER_FEATURE_DEBUG = false;
    }

#endif // DYNA_RANGE_DEBUG_MODE