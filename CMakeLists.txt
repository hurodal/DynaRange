cmake_minimum_required(VERSION 3.16)
project(dynRange CXX)

# Add our local modules directory to CMake's search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- BÚSQUEDA DE DEPENDENCIAS ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBRAW REQUIRED libraw)
find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(CLI11 REQUIRED)
find_package(Gettext REQUIRED)
find_package(wxWidgets REQUIRED)


# --- DEFINICIÓN DE TARGETS ---

# TARGET 1: La librería estática (El motor)
add_library(dynrange_core STATIC
    core/arguments.cpp
    core/functions.cpp
    core/engine.cpp 
)
target_include_directories(dynrange_core PUBLIC
    ${PROJECT_SOURCE_DIR}
    core
    ${LIBRAW_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${GETTEXT_INCLUDE_DIRS}
)
target_link_libraries(dynrange_core PUBLIC
    ${LIBRAW_LIBRARIES}
    ${OpenCV_LIBS}
    CLI11::CLI11
    ${GETTEXT_LIBRARIES}
)

# TARGET 2: El ejecutable de línea de comandos (Frontend CLI)
add_executable(dynRange dynRange.cpp)
target_link_libraries(dynRange PRIVATE dynrange_core)
target_include_directories(dynRange PRIVATE core)


# TARGET 3: El ejecutable de la GUI (Frontend GUI)
add_executable(dynRangeGui
    dynRangeGui.cpp
    core/gui/InputTab.cpp
    core/gui/LogTab.cpp
    core/gui/ResultsTab.cpp
)
target_include_directories(dynRangeGui PRIVATE core)
target_link_libraries(dynRangeGui PRIVATE 
    dynrange_core 
    ${wxWidgets_LIBRARIES}
)
target_compile_definitions(dynRangeGui PRIVATE ${wxWidgets_DEFINITIONS})
target_include_directories(dynRangeGui PRIVATE ${wxWidgets_INCLUDE_DIRS})


# --- CONFIGURACIÓN DE GETTEXT ---
include(UseGettext)
set(GETTEXT_XGETTEXT_FLAGS "--from-code=UTF-8")
set(LANGUAGES "es" "de" "it" "fr" "pt" "ca")

set(I18N_SOURCES
    dynRange.cpp
    dynRangeGui.cpp
    core/arguments.cpp
    core/functions.cpp
    core/engine.cpp
    core/gui/InputTab.cpp
    core/gui/LogTab.cpp
    core/gui/ResultsTab.cpp
)
GETTEXT_CREATE_POT_FILE(${PROJECT_NAME} ${I18N_SOURCES})
GETTEXT_ADD_PO_FILES(${PROJECT_NAME} ALL ${LANGUAGES})
GETTEXT_PROCESS_PO_FILES(${PROJECT_NAME} ALL INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/locale ${LANGUAGES})


# --- OPCIONAL: CONFIGURACIÓN DE INSTALADOR ---
include(InstallRequiredSystemLibraries)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
install(TARGETS dynRange dynRangeGui DESTINATION bin)
include(CPack)