# =============================================================================
# 1. CONFIGURACIÓN INICIAL DEL PROYECTO
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(dynaRange CXX)
set(PROJECT_VERSION "1.0.0")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
set(PORTABLE_DIR ${CMAKE_BINARY_DIR})


# =============================================================================
# 2. BÚSQUEDA DE DEPENDENCIAS
# =============================================================================
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBRAW REQUIRED libraw)
find_package(OpenCV 4 REQUIRED)
find_package(Eigen3 3.3 REQUIRED)
find_package(CLI11 REQUIRED)
find_package(Gettext REQUIRED)
set(GETTEXT_EXTRA_LIBS)
if(WIN32)
    list(APPEND GETTEXT_EXTRA_LIBS intl)
endif()
pkg_check_modules(CAIRO REQUIRED cairo)


# =============================================================================
# 3. AGRUPACIÓN DE FICHEROS FUENTE
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
# 4. DEFINICIÓN DE LOS EJECUTABLES (TARGETS)
# =============================================================================

# --- Target 1: Ejecutable de línea de comandos (CLI) ---
add_executable(rango src/rango.cpp ${CORE_SOURCES} icono_owl.rc)
target_include_directories(rango PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(rango PRIVATE 
    ${OpenCV_LIBS} ${LIBRAW_LIBRARIES} CLI11::CLI11
    ${GETTEXT_LIBRARIES} ${GETTEXT_EXTRA_LIBS} ${CAIRO_LIBRARIES}
)

# --- Target 2: Ejecutable con interfaz gráfica (GUI) ---
add_executable(dynaRangeGui WIN32
    src/DynaRangeGuiApp.cpp
    src/gui/DynaRangeBase.cpp
    src/gui/DynaRangeFrame.cpp
    src/gui/GuiPresenter.cpp    
    ${CORE_SOURCES}
    icono_noise.rc
)
target_include_directories(dynaRangeGui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- Lógica de enlazado para wxWidgets ---
# Si estamos en Windows Y en un entorno MSYS (como en GitHub Actions)...
if(WIN32 AND ENV{MSYSTEM})
    message(STATUS "MSYS environment detected. Using wx-config for wxWidgets flags.")
    # Busca el ejecutable wx-config
    find_program(WX_CONFIG_EXECUTABLE wx-config)
    if(NOT WX_CONFIG_EXECUTABLE)
        message(FATAL_ERROR "wx-config not found!")
    endif()

    # Ejecuta wx-config para obtener los flags de compilación y quita el salto de línea
    execute_process(
        COMMAND ${WX_CONFIG_EXECUTABLE} --cxxflags
        OUTPUT_VARIABLE WX_CXXFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # Ejecuta wx-config para obtener los flags de enlazado y quita el salto de línea
    execute_process(
        COMMAND ${WX_CONFIG_EXECUTABLE} --libs all
        OUTPUT_VARIABLE WX_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # Aplica los flags al target
    target_compile_options(dynaRangeGui PRIVATE ${WX_CXXFLAGS})
    target_link_libraries(dynaRangeGui PRIVATE ${WX_LIBS})
else()
    # Si no, estamos en un entorno normal (como tu Linux local), usamos el método estándar
    message(STATUS "Standard environment detected. Using find_package for wxWidgets.")
    find_package(wxWidgets COMPONENTS core base html xrc qa net REQUIRED)
    target_include_directories(dynaRangeGui PRIVATE ${wxWidgets_INCLUDE_DIRS})
    target_compile_definitions(dynaRangeGui PRIVATE ${wxWidgets_DEFINITIONS})
    target_link_libraries(dynaRangeGui PRIVATE ${wxWidgets_LIBRARIES})
endif()

# Enlazar el resto de librerías comunes
target_link_libraries(dynaRangeGui PRIVATE
    ${OpenCV_LIBS} ${LIBRAW_LIBRARIES} CLI11::CLI11
    ${GETTEXT_LIBRARIES} ${GETTEXT_EXTRA_LIBS} ${CAIRO_LIBRARIES}
)

# =============================================================================
# 5. CONFIGURACIÓN POST-COMPILACIÓN Y RECURSOS
# =============================================================================
set_target_properties(rango dynaRangeGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PORTABLE_DIR}")
file(COPY "${CMAKE_SOURCE_DIR}/logo.png" DESTINATION "${PORTABLE_DIR}")
file(COPY "${CMAKE_SOURCE_DIR}/favicon_noise.ico" DESTINATION "${PORTABLE_DIR}")

# =============================================================================
# 6. INTERNACIONALIZACIÓN Y EMPAQUETADO
# =============================================================================
include(UseGettext)
set(GETTEXT_XGETTEXT_FLAGS "--from-code=UTF-8")
set(LANGUAGES "es" "de" "it" "fr" "pt" "ca")
set(I18N_SOURCES src/rango.cpp src/DynaRangeGuiApp.cpp src/gui/DynaRangeFrame.cpp ${CORE_SOURCES})
GETTEXT_CREATE_POT_FILE(${PROJECT_NAME} ${I18N_SOURCES})
GETTEXT_ADD_PO_FILES(${PROJECT_NAME} ALL ${LANGUAGES})
GETTEXT_PROCESS_PO_FILES(${PROJECT_NAME} ALL INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/locale ${LANGUAGES})
include(InstallRequiredSystemLibraries)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
install(TARGETS rango dynaRangeGui DESTINATION bin)
include(CPack)