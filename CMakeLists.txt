# =============================================================================
# 1. CONFIGURACIÓN INICIAL DEL PROYECTO
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(dynaRange CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(PROJECT_VERSION "1.1.1")
cmake_policy(SET CMP0177 NEW)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# 2. CONFIGURACIÓN ESPECÍFICA DE LA PLATAFORMA
# =============================================================================
# (Vacía)

# =============================================================================
# 3. DEPENDENCIAS
# =============================================================================
find_package(PkgConfig REQUIRED)

# --- Dependencias Comunes (CLI y GUI) ---
pkg_check_modules(LIBRAW REQUIRED libraw)
find_package(OpenCV 4 REQUIRED)
find_package(CLI11 REQUIRED)
find_package(Gettext REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

set(GETTEXT_EXTRA_LIBS)
if(WIN32)
    list(APPEND GETTEXT_EXTRA_LIBS intl)
endif()

# --- Dependencias Solo para la GUI ---
find_package(wxWidgets COMPONENTS core base html xrc qa net REQUIRED)
set(GUI_WX_LIBS ${wxWidgets_LIBRARIES})

# La buscamos siempre, pero solo la incluimos si es necesario
find_package(Eigen3 REQUIRED)

# =============================================================================
# OPCIÓN PARA SELECCIONAR EL ESTIMADOR -> ELIMINADA
# =============================================================================
# option(DYNA_RANGE_USE_LBFGSPP "Use LBFGSpp estimator (Method 2)" ON) # ELIMINADA

# =============================================================================
# CONFIGURACIÓN DE LBFGSPP (HEADER-ONLY)
# =============================================================================
set(LBFGSPP_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/LBFGSpp/include)
# Comprobar si existe solo si vamos a usarlo (determinado por la constante C++)
# Esta comprobación es más informativa que funcional ahora
if(EXISTS ${CMAKE_SOURCE_DIR}/src/core/math/estimation/Constants.hpp)
    file(READ ${CMAKE_SOURCE_DIR}/src/core/math/estimation/Constants.hpp LBFGSPP_CONST_CONTENT)
    string(REGEX MATCH "constexpr bool USE_LBFGSPP_ESTIMATOR = true" LBFGSPP_ENABLED ${LBFGSPP_CONST_CONTENT})
    if(LBFGSPP_ENABLED)
        message(STATUS "LBFGSpp selected via C++ constant. Checking headers...")
        if(NOT EXISTS ${LBFGSPP_INCLUDE_DIR}/LBFGS.h)
             message(FATAL_ERROR "LBFGSpp header LBFGS.h not found directly in ${LBFGSPP_INCLUDE_DIR}")
        endif()
        message(STATUS "LBFGSpp headers found.")
    else()
        message(STATUS "Gradient Descent selected via C++ constant.")
    endif()
else()
     message(WARNING "src/core/math/estimation/Constants.hpp not found. Cannot determine estimator setting.")
     set(LBFGSPP_ENABLED FALSE) # Asumir que no se usa LBFGSpp si el fichero no existe
endif()

# =============================================================================
# 4. FUENTES
# =============================================================================
set(CORE_SOURCES
    src/core/analysis/Analysis.cpp
    src/core/analysis/CurveCalculator.cpp
    src/core/analysis/ImageAnalyzer.cpp
    src/core/analysis/RawProcessor.cpp
    src/core/arguments/ArgumentManager.cpp
    src/core/arguments/ChartOptionsParser.cpp
    src/core/arguments/parsing/ArgumentRegistry.cpp
    src/core/arguments/parsing/CliParser.cpp
    src/core/arguments/parsing/OptionsConverter.cpp
    src/core/artifacts/ArtifactFactory.cpp
    src/core/artifacts/data/ReportWriter.cpp
    src/core/artifacts/image/DebugImageWriter.cpp
    src/core/artifacts/plot/PlotWriter.cpp
    src/core/engine/Engine.cpp
    src/core/engine/Initialization.cpp
    src/core/engine/initialization/CalibrationHandler.cpp
    src/core/engine/initialization/ConfigReporter.cpp
    src/core/engine/initialization/FileSorter.cpp
    src/core/engine/initialization/InputFileFilter.cpp
    src/core/engine/initialization/PreAnalysisRawSelector.cpp
    src/core/engine/PatchAnalysisStrategy.cpp
    src/core/engine/processing/AnalysisLoopRunner.cpp
    src/core/engine/processing/CornerDetectionHandler.cpp
    src/core/engine/processing/Processing.cpp
    src/core/engine/processing/ResultAggregator.cpp
    src/core/engine/Reporting.cpp
    src/core/engine/Validation.cpp
    src/core/graphics/detection/ChartCornerDetector.cpp
    src/core/graphics/drawing/AxisDrawer.cpp
    src/core/graphics/drawing/AxisLabelDrawer.cpp
    src/core/graphics/drawing/CurveDrawer.cpp
    src/core/graphics/drawing/FooterDrawer.cpp
    src/core/graphics/drawing/GridDrawer.cpp
    src/core/graphics/drawing/LabelDrawer.cpp
    src/core/graphics/drawing/TitleDrawer.cpp
    src/core/graphics/FontManager.cpp
    src/core/graphics/geometry/KeystoneCorrection.cpp
    src/core/graphics/ImageProcessing.cpp
    src/core/graphics/PlotBase.cpp
    src/core/graphics/PlotBoundsCalculator.cpp
    src/core/graphics/PlotData.cpp
    src/core/graphics/PlotDataGenerator.cpp
    src/core/graphics/PlotInfoBox.cpp
    src/core/graphics/PlotOrchestrator.cpp
    src/core/io/OutputWriter.cpp
    src/core/io/raw/RawFile.cpp
    src/core/io/raw/RawImageAccessor.cpp
    src/core/io/raw/RawLoader.cpp
    src/core/io/raw/RawMetadataExtractor.cpp
    src/core/math/estimation/gradient_descent.cpp
    src/core/math/estimation/lbfgspp_optimizer.cpp
    src/core/math/estimation/TruncatedNormalEstimator.cpp
    src/core/math/Math.cpp
    src/core/setup/CalibrationEstimator.cpp
    src/core/setup/ChartProfile.cpp
    src/core/setup/InputFileManager.cpp
    src/core/setup/MetadataExtractor.cpp
    src/core/setup/PlotLabelGenerator.cpp
    src/core/setup/PreAnalysis.cpp
    src/core/setup/PreAnalysisManager.cpp
    src/core/setup/SensorResolution.cpp
    src/core/utils/Base64Encode.cpp
    src/core/utils/CommandGenerator.cpp
    src/core/utils/Formatters.cpp
    src/core/utils/LocaleManager.cpp
    src/core/utils/OutputFilenameGenerator.cpp
    src/core/utils/PathManager.cpp
    src/core/utils/PlatformUtils.cpp
    src/core/utils/PlotTitleGenerator.cpp
)

# =============================================================================
# 5. TARGETS
# =============================================================================
add_executable(rango
    src/cli/rango.cpp
    ${CORE_SOURCES}
)

if(WIN32)
    target_sources(rango PRIVATE assets/windows/icono_owl.rc)
endif()

# Incluir directorios
target_include_directories(rango PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${LIBRAW_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/arguments
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/analysis
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/setup
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/io
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/math
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/math/estimation
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/artifacts # *** NUEVA LÍNEA ***
    ${EIGEN3_INCLUDE_DIR}
    ${LBFGSPP_INCLUDE_DIR}
)

add_executable(dynaRangeGui WIN32
    src/gui/controllers/ChartController.cpp
    src/gui/controllers/InputController.cpp
    src/gui/controllers/LogController.cpp
    src/gui/controllers/ResultsController.cpp
    src/gui/controllers/PreviewController.cpp
    src/gui/DynaRangeFrame.cpp
    src/gui/DynaRangeGuiApp.cpp
    src/gui/generated/DynaRangeBase.cpp
    src/gui/GuiPresenter.cpp
    src/gui/helpers/CvWxImageConverter.cpp
    src/gui/helpers/GuiPlotter.cpp
    src/gui/helpers/RawExtensionHelper.cpp
    src/gui/helpers/ResultsGridManager.cpp
    src/gui/preview_interaction/ChartCornerInteractor.cpp
    src/gui/preview_interaction/PreviewOverlayRenderer.cpp
    ${CORE_SOURCES}
)

if(WIN32)
    target_sources(dynaRangeGui PRIVATE assets/windows/icono_noise.rc)
endif()

if(APPLE)
    set_target_properties(dynaRangeGui PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/assets/macos/icono.icns
    )
endif()

# Incluir directorios
target_include_directories(dynaRangeGui PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${LIBRAW_INCLUDE_DIRS}
    ${wxWidgets_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/arguments
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/analysis
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/setup
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/io
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/math
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/math/estimation
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/artifacts # *** NUEVA LÍNEA ***
    ${EIGEN3_INCLUDE_DIR}
    ${LBFGSPP_INCLUDE_DIR}
)

# target_compile_definitions(dynaRangeGui PRIVATE ${wxWidgets_DEFINITIONS} ${ESTIMATOR_DEFINE})
target_compile_definitions(dynaRangeGui PRIVATE ${wxWidgets_DEFINITIONS})


# Librerías Windows
if(WIN32)
    set(WIN_LINK_LIBS
        gdi32 comdlg32 shell32 ole32 oleaut32 uuid ws2_32 rpcrt4 shlwapi
        version comctl32 uxtheme oleacc advapi32
        png jpeg tiff z iconv
        gomp
    )
endif()

# Enlazado rango
target_link_libraries(rango PRIVATE
    ${OpenCV_LIBS}
    ${LIBRAW_LIBRARIES}
    CLI11::CLI11
    ${GETTEXT_LIBRARIES}
    ${GETTEXT_EXTRA_LIBS}
    ${CAIRO_LIBRARIES}
    ${WIN_LINK_LIBS}
)

# Enlazado dynaRangeGui
target_link_libraries(dynaRangeGui PRIVATE
    ${GUI_WX_LIBS}
    ${OpenCV_LIBS}
    ${LIBRAW_LIBRARIES}
    CLI11::CLI11
    ${GETTEXT_LIBRARIES}
    ${GETTEXT_EXTRA_LIBS}
    ${CAIRO_LIBRARIES}
    ${WIN_LINK_LIBS}
)

# =============================================================================
# 6. ASSET MANAGEMENT
# =============================================================================
file(COPY assets/images DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets)