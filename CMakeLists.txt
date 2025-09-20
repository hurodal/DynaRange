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
set(PKG_CONFIG_USE_STATIC ON)

pkg_check_modules(LIBRAW REQUIRED libraw)
find_package(OpenCV 4 REQUIRED)
find_package(Eigen3 3.3 REQUIRED)
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
    src/core/arguments/CommandGenerator.cpp
    src/core/arguments/CommandLineParser.cpp
    src/core/ChartProfile.cpp
    src/core/engine/Engine.cpp
    src/core/engine/Initialization.cpp
    src/core/engine/Processing.cpp
    src/core/engine/Reporting.cpp
    src/core/engine/Validation.cpp
    src/core/graphics/ImageProcessing.cpp
    src/core/graphics/PlotBase.cpp
    src/core/graphics/PlotData.cpp
    src/core/graphics/PlotInfoBox.cpp
    src/core/graphics/Plotting.cpp
    src/core/io/RawFile.cpp
    src/core/math/Math.cpp
    src/core/setup/FileSorter.cpp
    src/core/setup/MetadataExtractor.cpp
    src/core/setup/PlotLabelGenerator.cpp
    src/core/setup/SensorResolution.cpp
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

# --- ENLAZADO DE LIBRERÍAS (CON LA SOLUCIÓN DEL LOG) ---
if(WIN32)
    # Esta es la lista completa basada en los errores de "undefined reference" del log
    set(WIN_STATIC_LIBS
        gdi32 comdlg32 shell32 ole32 oleaut32 uuid ws2_32 rpcrt4 shlwapi
        version comctl32 uxtheme oleacc advapi32
        # Dependencias de imágenes y otras librerías
        png jpeg tiff z iconv
        # Dependencia de OpenMP
        gomp
    )
endif()

target_link_libraries(rango PRIVATE
    ${OpenCV_LIBS}
    ${LIBRAW_LIBRARIES}
    CLI11::CLI11
    ${GETTEXT_LIBRARIES}
    ${GETTEXT_EXTRA_LIBS}
    ${CAIRO_LIBRARIES}
    ${WIN_STATIC_LIBS}
)

target_link_libraries(dynaRangeGui PRIVATE
    ${wxWidgets_LIBRARIES}
    ${OpenCV_LIBS}
    ${LIBRAW_LIBRARIES}
    CLI11::CLI11
    ${GETTEXT_LIBRARIES}
    ${GETTEXT_EXTRA_LIBS}
    ${CAIRO_LIBRARIES}
    ${WIN_STATIC_LIBS}
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
    ${CORE_SOURCES}
)

GETTEXT_CREATE_POT_FILE(${PROJECT_NAME} ${I18N_SOURCES})
GETTEXT_ADD_PO_FILES(${PROJECT_NAME} ALL ${LANGUAGES})
GETTEXT_PROCESS_PO_FILES(${PROJECT_NAME} ALL INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/locale ${LANGUAGES})

# =============================================================================
# 8. INSTALACIÓN Y EMPAQUETADO (CPack)
# =============================================================================
# --- Instalar los ejecutables ---
install(TARGETS rango dynaRangeGui
    RUNTIME DESTINATION bin
    COMPONENT Applications
)

# --- MODIFICACIÓN 1: Instalar las DLLs de forma eficiente para evitar el cuelgue ---
# 1. Creamos una lista con todas las DLLs que el workflow copió al directorio de compilación.
file(GLOB DLL_FILES "${CMAKE_BINARY_DIR}/*.dll")

# 2. Le decimos a CPack que instale ESA LISTA específica de archivos.
#    Esto es mucho más rápido y fiable que escanear todo el directorio con "install(DIRECTORY...)".
if(DLL_FILES)
    install(FILES ${DLL_FILES}
        DESTINATION bin
        COMPONENT Applications
    )
endif()

# --- Instalar otros archivos (iconos, etc.) ---
install(FILES
    "${CMAKE_SOURCE_DIR}/logo.png"
    "${CMAKE_SOURCE_DIR}/favicon_noise.ico"
    DESTINATION bin
    COMPONENT Applications
)

# --- Configurar CPack para generar un instalador MSI ---
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "TuNombre o TuEmpresa")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DynaRange - Herramienta de análisis de rango dinámico")
set(CPACK_GENERATOR "WIX")
set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://github.com/hurodal/DynaRange")

# --- Lógica para añadir la carpeta 'bin' al PATH del sistema ---
set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/AddToPath.wxs")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/AddToPath.wxs"
[[<?xml version="1.0" encoding="utf-8"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
    <Fragment>
        <DirectoryRef Id="INSTALLDIR">
            <Component Id="AddToPath" Guid="{A1B2C3D4-E5F6-7890-A1B2-C3D4E5F67890}" KeyPath="yes">
                <Environment Id="PATH" Name="PATH" Value="[INSTALLDIR]bin" Permanent="no" Part="last" Action="set" System="yes" />
            </Component>
        </DirectoryRef>
    </Fragment>
</Wix>]]
)

set(CPACK_WIX_EXTRA_FEATURES
[[<Feature Id="AddToPathFeature" Title="Add to PATH" Description="Add application directory to system PATH" Level="1">
    <ComponentRef Id="AddToPath"/>
</Feature>]]
)

include(CPack)