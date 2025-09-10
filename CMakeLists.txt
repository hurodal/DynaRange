cmake_minimum_required(VERSION 3.16)
project(dynaRange CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBRAW REQUIRED libraw)
find_package(OpenCV 4 REQUIRED)
find_package(Eigen3 3.3 REQUIRED)
find_package(CLI11 REQUIRED)
find_package(Gettext REQUIRED)
find_package(wxWidgets REQUIRED)

# Find the Cairo library
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

# --- Definición de la carpeta de salida para el instalador ---
# Todos los archivos necesarios se copiarán a "build"
set(PORTABLE_DIR ${CMAKE_BINARY_DIR})

set(CORE_SOURCES
    src/core/Analysis.cpp
    src/core/Arguments.cpp
    src/core/Engine.cpp
    src/core/engine/Initialization.cpp
    src/core/engine/Processing.cpp
    src/core/engine/Reporting.cpp
    src/core/graphics/Drawing.cpp
    src/core/graphics/Plotting.cpp
    src/core/Math.cpp
    src/core/RawFile.cpp
)

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
)
target_link_libraries(rango PRIVATE 
    ${OpenCV_LIBS} 
    ${LIBRAW_LIBRARIES}
    CLI11::CLI11
    ${GETTEXT_LIBRARIES}
    intl
    ${CAIRO_LIBRARIES}
)

add_executable(dynaRangeGui WIN32
    src/DynaRangeGuiApp.cpp
    src/gui/DynaRangeBase.cpp
    src/gui/DynaRangeFrame.cpp
    ${CORE_SOURCES}
    icono_noise.rc
)
target_include_directories(dynaRangeGui PRIVATE 
    ${OpenCV_INCLUDE_DIRS}
    ${LIBRAW_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${wxWidgets_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
)
target_link_libraries(dynaRangeGui PRIVATE
    ${wxWidgets_LIBRARIES}
    ${OpenCV_LIBS}
    ${LIBRAW_LIBRARIES}
    CLI11::CLI11
    ${GETTEXT_LIBRARIES}
    intl
    ${CAIRO_LIBRARIES}
)

# --- Establecer el directorio de salida de los ejecutables ---
set_target_properties(rango dynaRangeGui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PORTABLE_DIR}"
)

# --- Copiar archivos de recursos a la carpeta portable ---
# Asegúrate de que el fichero logo.png está en la raíz del proyecto
file(COPY "${CMAKE_SOURCE_DIR}/logo.png" DESTINATION "${PORTABLE_DIR}")


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

include(InstallRequiredSystemLibraries)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
install(TARGETS rango dynaRangeGui DESTINATION bin)
include(CPack)