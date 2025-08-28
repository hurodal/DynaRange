cmake_minimum_required(VERSION 3.16)
project(dynamicrange CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# LibRaw via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBRAW REQUIRED libraw)

# OpenCV 4
find_package(OpenCV 4 REQUIRED)

# Eigen3
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

# CLI11 (Añadido para el nuevo parser de argumentos)
find_package(CLI11 REQUIRED)

# Ejecutable principal
add_executable(dynamicrange
    dynamicrange.cpp
    core/arguments.cpp
)

target_include_directories(dynamicrange PRIVATE
    ${LIBRAW_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    core
)

target_link_libraries(dynamicrange
    ${LIBRAW_LIBRARIES}
    ${OpenCV_LIBS}
    CLI11::CLI11
)

# Si pkg-config aporta flags extra para LibRaw:
target_compile_options(dynamicrange PRIVATE ${LIBRAW_CFLAGS_OTHER})
