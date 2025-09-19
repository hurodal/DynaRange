# =============================================================================
# 1. CONFIGURACIÓN INICIAL DEL PROYECTO
# =============================================================================
# Requiere una versión mínima de CMake (3.16 es una buena base moderna).
cmake_minimum_required(VERSION 3.16)

# Define el proyecto, su nombre y el lenguaje principal (C++).
project(dynaRange CXX)
set(PROJECT_VERSION "1.0.0")

# --- Opciones y variables globales del compilador ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Añade definiciones globales para compatibilidad, especialmente en Windows.
# -DNOMINMAX es útil para evitar conflictos con min/max en <windows.h>
# -D_USE_MATH_DEFINES asegura que constantes como M_PI estén disponibles.
add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)

# --- Rutas y directorios ---
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Directorio de salida para los ejecutables y recursos.
# CMAKE_BINARY_DIR es el directorio 'build' que creamos.
set(PORTABLE_DIR ${CMAKE_BINARY_DIR})


# =============================================================================
# 2. CONFIGURACIÓN ESPECÍFICA DE LA PLATAFORMA (ANTES DE BUSCAR LIBRERÍAS)
# =============================================================================
# Configuración específica para Windows (MSYS2/MinGW)
if(WIN32)
    # CMAKE_PREFIX_PATH es CRUCIAL en MSYS2 para que CMake encuentre las librerías
    # instaladas por pacman (como wxWidgets, Gettext, etc.)
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")

    # CMake en Windows puede tener problemas con la detección de algunas funcionalidades
    # si no se establecen ciertas definiciones explícitamente.
    # Por ejemplo, para OpenMP, aunque OpenCV ya lo usa, a veces es necesario.
    # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
endif()


# =============================================================================
# 3. BÚSQUEDA DE DEPENDENCIAS
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

# --- Lógica ESPECÍFICA para wxWidgets ---
# Si estamos en Windows Y en un entorno MSYS (como en GitHub Actions)...
if(WIN32 AND ENV{MSYSTEM})
    message(STATUS "MSYS environment detected. Using wx-config for wxWidgets flags.")
    # Busca el ejecutable wx-config
    find_program(WX_CONFIG_EXECUTABLE wx-config)
    if(NOT WX_CONFIG_EXECUTABLE)
        message(FATAL_ERROR "wx-config not found! Make sure wxWidgets is installed.")
    endif()
    # Ejecuta wx-config para obtener los flags de compilación
    execute_process(
        COMMAND ${WX_CONFIG_EXECUTABLE} --cxxflags
        OUTPUT_VARIABLE WX_CXXFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # Ejecuta wx-config para obtener los flags de enlazado
    execute_process(
        COMMAND ${WX_CONFIG_EXECUTABLE} --libs all
        OUTPUT_VARIABLE WX_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    # Si no, estamos en un entorno normal, usamos el método estándar
    message(STATUS "Standard environment detected. Using find_package for wxWidgets.")
    find_package(wxWidgets COMPONENTS core base html xrc qa net REQUIRED)
endif()

pkg_check_modules(CAIRO REQUIRED cairo)

# =============================================================================
# 4. AGRUPACIÓN DE FICHEROS FUENTE
# =============================================================================
# Lista de ficheros fuente que son compartidos entre la versión CLI y la GUI.
# Esto evita duplicación y asegura que todos los componentes centrales se compilen.
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
# 5. DEFINICIÓN DE LOS EJECUTABLES (TARGETS)
# =============================================================================

# --- Target 1: Ejecutable de línea de comandos (CLI) ---
# Crea el ejecutable 'rango'.
add_executable(rango
    src/rango.cpp
    ${CORE_SOURCES}
    # Fichero de recursos para el icono y metadatos en Windows.
    # Esto se ignora en Linux.
    icono_owl.rc
)

# Define los directorios de inclusión (headers) para el compilador.
# 'PRIVATE' significa que solo el target 'rango' necesita estas rutas para compilarse.
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

# Define las librerías con las que el ejecutable 'rango' debe enlazarse.
# 'PRIVATE' significa que estas librerías solo son necesarias para el enlazado de 'rango'.
target_link_libraries(rango PRIVATE 
    ${OpenCV_LIBS} 
    ${LIBRAW_LIBRARIES}
    CLI11::CLI11
    ${GETTEXT_LIBRARIES}
    ${GETTEXT_EXTRA_LIBS} # Nuestra variable condicional para 'intl'
    ${CAIRO_LIBRARIES}
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

# --- Lógica de enlazado para wxWidgets ---
if(WIN32 AND ENV{MSYSTEM})
    message(STATUS "MSYS environment detected. Using wx-config for wxWidgets flags.")
    find_program(WX_CONFIG_EXECUTABLE wx-config)
    if(NOT WX_CONFIG_EXECUTABLE)
        message(FATAL_ERROR "wx-config not found!")
    endif()
    execute_process(
        COMMAND ${WX_CONFIG_EXECUTABLE} --cxxflags
        OUTPUT_VARIABLE WX_CXXFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${WX_CONFIG_EXECUTABLE} --libs all
        OUTPUT_VARIABLE WX_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    target_compile_options(dynaRangeGui PRIVATE ${WX_CXXFLAGS})
    target_link_libraries(dynaRangeGui PRIVATE ${WX_LIBS})
else()
    message(STATUS "Standard environment detected. Using find_package for wxWidgets.")
    find_package(wxWidgets COMPONENTS core base html xrc qa net REQUIRED)
    # --- ¡¡¡CORRECCIÓN AQUÍ!!! ---
    # Asegúrate de incluir EIGEN3 en los directorios de inclusión
    target_include_directories(dynaRangeGui PRIVATE
        ${OpenCV_INCLUDE_DIRS}
        ${LIBRAW_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIR} # <-- ¡ESTA LÍNEA FALTABA!
        ${wxWidgets_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/arguments
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/analysis
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/setup
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/io
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/graphics
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/utils
    )
    target_compile_definitions(dynaRangeGui PRIVATE ${wxWidgets_DEFINITIONS})
    target_link_libraries(dynaRangeGui PRIVATE ${wxWidgets_LIBRARIES})
endif()

# Enlazar el resto de librerías comunes (esto no cambia)
target_link_libraries(dynaRangeGui PRIVATE
    ${OpenCV_LIBS}
    ${LIBRAW_LIBRARIES}
    CLI11::CLI11
    ${GETTEXT_LIBRARIES}
    ${GETTEXT_EXTRA_LIBS}
    ${CAIRO_LIBRARIES}
)

# =============================================================================
# 6. CONFIGURACIÓN POST-COMPILACIÓN Y RECURSOS
# =============================================================================
# Establece el directorio de salida para los ejecutables de ambos targets.
# Esto hará que 'rango.exe' y 'dynaRangeGui.exe' aparezcan directamente en la carpeta 'build'.
set_target_properties(rango dynaRangeGui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PORTABLE_DIR}"
)

# Copia los recursos adicionales al directorio de salida.
file(COPY "${CMAKE_SOURCE_DIR}/logo.png" DESTINATION "${PORTABLE_DIR}")
file(COPY "${CMAKE_SOURCE_DIR}/favicon_noise.ico" DESTINATION "${PORTABLE_DIR}")

# =============================================================================
# 7. INTERNACIONALIZACIÓN (Traducciones con Gettext)
# =============================================================================
# Incluye el módulo de CMake para manejar Gettext.
include(UseGettext)

# Define los flags para xgettext, asegurando la codificación UTF-8.
set(GETTEXT_XGETTEXT_FLAGS "--from-code=UTF-8")

# Define los idiomas que se van a traducir.
set(LANGUAGES "es" "de" "it" "fr" "pt" "ca")

# Lista de ficheros fuente que contienen cadenas traducibles.
set(I18N_SOURCES
    src/rango.cpp
    src/DynaRangeGuiApp.cpp
    src/gui/DynaRangeFrame.cpp
    ${CORE_SOURCES} # Incluye los ficheros comunes también
)

# Genera el fichero .pot (Portable Object Template).
GETTEXT_CREATE_POT_FILE(${PROJECT_NAME} ${I18N_SOURCES})

# Añade los ficheros .po (Portable Object) para cada idioma.
GETTEXT_ADD_PO_FILES(${PROJECT_NAME} ALL ${LANGUAGES})

# Procesa los ficheros .po y los instala en la ubicación estándar de locales.
GETTEXT_PROCESS_PO_FILES(${PROJECT_NAME} ALL INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/locale ${LANGUAGES})


# =============================================================================
# 8. INSTALACIÓN Y EMPAQUETADO (CPack)
# =============================================================================
# Incluye las librerías del sistema necesarias para el instalador (Windows).
include(InstallRequiredSystemLibraries)

# Define el nombre y la versión del proyecto para CPack.
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

# Instala los ejecutables en el directorio 'bin' del destino de instalación.
install(TARGETS rango dynaRangeGui DESTINATION bin)

# Incluye CPack para generar paquetes de instalación.
include(CPack)