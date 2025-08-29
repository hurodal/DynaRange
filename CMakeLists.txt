cmake_minimum_required(VERSION 3.16)
project(dynamicrange CXX)

# Add our local modules directory to CMake's search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# LibRaw via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBRAW REQUIRED libraw)

# OpenCV 4
find_package(OpenCV 4 REQUIRED)

# Eigen3
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

# CLI11 (Added for the new argument parser)
find_package(CLI11 REQUIRED)

# --- STEP 1: DEFINE THE EXECUTABLE AND ITS SOURCES ---
add_executable(dynamicrange
    dynamicrange.cpp
    core/arguments.cpp
    core/functions.cpp
)

# --- STEP 2: CONFIGURE GETTEXT (IT NOW KNOWS THE SOURCES) ---
find_package(Gettext REQUIRED)
include(UseGettext)
set(GETTEXT_XGETTEXT_FLAGS "--from-code=UTF-8")

# Define the languages you will translate to
set(LANGUAGES "es" "de" "it" "fr" "pt")

# Create the .pot template from the project's sources
GETTEXT_CREATE_POT_FILE(
    ${PROJECT_NAME}
    ALL # "ALL" will now find the sources defined in add_executable
)

# Add the .po files for each language
GETTEXT_ADD_PO_FILES(
    ${PROJECT_NAME}
    ALL
    ${LANGUAGES}
)

# Create a "target" to compile .po to .mo files and install them
GETTEXT_PROCESS_PO_FILES(
    ${PROJECT_NAME}
    ALL
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/locale
    ${LANGUAGES}
)

# --- STEP 3: LINK LIBRARIES AND DIRECTORIES ---
target_include_directories(dynamicrange PRIVATE
    ${LIBRAW_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    core
    ${GETTEXT_INCLUDE_DIRS}
)

target_link_libraries(dynamicrange
    ${LIBRAW_LIBRARIES}
    ${OpenCV_LIBS}
    CLI11::CLI11
    ${GETTEXT_LIBRARIES}
)

# If pkg-config provides extra flags for LibRaw:
target_compile_options(dynamicrange PRIVATE ${LIBRAW_CFLAGS_OTHER})