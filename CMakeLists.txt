# =============================================================================
# 1. CONFIGURACIÓN INICIAL DEL PROYECTO
# =============================================================================
cmake_minimum_required(VERSION 3.16)

project(dynaRange CXX)
set(PROJECT_VERSION "1.0.0")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(PORTABLE_DIR ${CMAKE_BINARY_DIR})

# =============================================================================
# 2. CONFIGURACIÓN ESPECÍFICA DE LA PLATAFORMA
# =============================================================================
# Esta sección debe estar vacía porque la ruta se pasa desde el build.yml

# =============================================================================
# 3. DEPENDENCIAS
# =============================================================================
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBRAW REQUIRED libraw)
find_package(OpenCV 4 REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(CLI11 REQUIRED)

find_package(Gettext REQUIRED)
set(GETTEXT_EXTRA_LIBS)
if(WIN32)
    list(APPEND GETTEXT_EXTRA_LIBS intl)
endif()

# --- wxWidgets ---
find_package(wxWidgets COMPONENTS core base html xrc qa net REQUIRED)

pkg_check_modules(CAIRO REQUIRED cairo)

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
    src/core/ChartProfile.cpp
    src/core/engine/Engine.cpp
    src/core/engine/Initialization.cpp
    src/core/engine/Processing.cpp
    src/core/engine/Reporting.cpp
    src/core/engine/Validation.cpp
    src/core/graphics/ChartGenerator.cpp
    src/core/graphics/ImageProcessing.cpp
    src/core/graphics/ImageProcessing.cpp
    src/core/graphics/PlotBase.cpp
    src/core/graphics/PlotData.cpp
    src/core/graphics/PlotInfoBox.cpp
    src/core/graphics/Plotting.cpp
    src/core/io/OutputWriter.cpp
    src/core/io/RawFile.cpp
    src/core/math/Math.cpp
    src/core/setup/FileSorter.cpp
    src/core/setup/MetadataExtractor.cpp
    src/core/setup/PlotLabelGenerator.cpp
    src/core/setup/SensorResolution.cpp
    src/core/utils/LocaleManager.cpp
    src/core/utils/PathManager.cpp
)

# =============================================================================
# 5. TARGETS
# =============================================================================
add_executable(rango
    src/rango.cpp
    ${CORE_SOURCES}
    icono_owl.rc
)

target_include_directories(rango PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${LIBRAW_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${CAIRO_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/arguments
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/analysis
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/setup
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/io
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/utils
)

add_executable(dynaRangeGui WIN32
    src/DynaRangeGuiApp.cpp
    src/gui/DynaRangeBase.cpp
    src/gui/DynaRangeFrame.cpp
    src/gui/GuiPresenter.cpp
    src/gui/ImageViewer.cpp
    src/gui/ResultsGridManager.cpp
    src/gui/InputController.cpp
    src/gui/LogController.cpp
    src/gui/ResultsController.cpp
    ${CORE_SOURCES}
    icono_noise.rc
)

target_include_directories(dynaRangeGui PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${LIBRAW_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${wxWidgets_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/arguments
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/analysis
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/setup
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/io
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/utils
)

target_compile_definitions(dynaRangeGui PRIVATE
    ${wxWidgets_DEFINITIONS}
)

# Contiene librerías de importación para DLLs del sistema y de dependencias.
if(WIN32)
    set(WIN_LINK_LIBS
        gdi32 comdlg32 shell32 ole32 oleaut32 uuid ws2_32 rpcrt4 shlwapi
        version comctl32 uxtheme oleacc advapi32
        png jpeg tiff z iconv
        gomp
    )
endif()

target_link_libraries(rango PRIVATE
    ${OpenCV_LIBS}
    ${LIBRAW_LIBRARIES}
    CLI11::CLI11
    Eigen3::Eigen
    ${GETTEXT_LIBRARIES}
    ${GETTEXT_EXTRA_LIBS}
    ${CAIRO_LIBRARIES}
    ${WIN_LINK_LIBS}
)

target_link_libraries(dynaRangeGui PRIVATE
    ${wxWidgets_LIBRARIES}
    ${OpenCV_LIBS}
    ${LIBRAW_LIBRARIES}
    CLI11::CLI11
    Eigen3::Eigen
    ${GETTEXT_LIBRARIES}
    ${GETTEXT_EXTRA_LIBS}
    ${CAIRO_LIBRARIES}
    ${WIN_LINK_LIBS}
)

# =============================================================================
# 6. POST-BUILD
# =============================================================================
set_target_properties(rango dynaRangeGui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PORTABLE_DIR}"
)

# =============================================================================
# 7. INTERNACIONALIZACIÓN
# =============================================================================
include(UseGettext)
set(GETTEXT_XGETTEXT_FLAGS "--from-code=UTF-8")
set(LANGUAGES "es" "de" "it" "fr" "pt" "ca")
set(I18N_SOURCES
    src/rango.cpp
    src/DynaRangeGuiApp.cpp
    src/gui/DynaRangeFrame.cpp
    src/gui/DynaRangeBase.cpp
    src/gui/GuiPresenter.cpp
    src/gui/ImageViewer.cpp
    src/gui/ResultsGridManager.cpp
    src/gui/InputController.cpp
    src/gui/LogController.cpp
    src/gui/ResultsController.cpp
    ${CORE_SOURCES}
)
GETTEXT_CREATE_POT_FILE(${PROJECT_NAME} ${I18N_SOURCES})
GETTEXT_ADD_PO_FILES(${PROJECT_NAME} ALL ${LANGUAGES})
GETTEXT_PROCESS_PO_FILES(${PROJECT_NAME} ALL INSTALL_DESTINATION "share/locale" ${LANGUAGES})

# =============================================================================
# 8. INSTALACIÓN Y EMPAQUETADO (CPack)
# =============================================================================

# Este bloque completo solo se ejecutará si estamos dentro de un entorno de GitHub Actions.
if($ENV{GITHUB_ACTIONS})

    # --- Definir qué ficheros se instalan ---
    install(TARGETS rango dynaRangeGui
        RUNTIME DESTINATION bin
    )
    install(FILES "${CMAKE_SOURCE_DIR}/logo.png"
        DESTINATION bin
    )
    install(CODE "
        message(STATUS \"Buscando DLLs en el directorio de compilación: ${CMAKE_BINARY_DIR}\")
        file(GLOB DLL_FILES \"${CMAKE_BINARY_DIR}/*.dll\")
        if(DLL_FILES)
            message(STATUS \"DLLs encontradas, instalando en el directorio bin del paquete.\")
            file(INSTALL \${DLL_FILES} DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\")
        else()
            message(WARNING \"No se encontraron DLLs en ${CMAKE_BINARY_DIR} para instalar.\")
        endif()
    ")

    # --- Configuración General de CPack ---
    set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_VENDOR "DynaRange Project")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Herramienta de análisis de rango dinámico")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "DynaRange")
    set(CPACK_PACKAGE_FILE_NAME "dynaRange-installer")
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "DynaRange")
    set(CPACK_PACKAGE_CONTACT "github.com/hurodal/DynaRange")

    # --- Configuración Específica para WiX (Instalador de Windows) ---
    set(CPACK_GENERATOR "WIX")
    set(CPACK_WIX_PRODUCT_GUID "FC1B47A8-6548-439D-9277-2415A5818953")
    set(CPACK_WIX_UPGRADE_GUID "e8b9a5b0-3132-4e0f-90e8-ac099516a541")

    # --- Usar la plantilla WiX ---
    configure_file(
        "${CMAKE_SOURCE_DIR}/wix_template.wxs.in"
        "${CMAKE_BINARY_DIR}/wix_template.wxs"
        @ONLY
    )
    set(CPACK_WIX_TEMPLATE "${CMAKE_BINARY_DIR}/wix_template.wxs")


    include(CPack)

endif()