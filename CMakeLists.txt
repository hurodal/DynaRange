# =============================================================================
# 1. CONFIGURACIÓN INICIAL DEL PROYECTO
# =============================================================================
# Requiere una versión mínima de CMake (3.16 es una buena base moderna).
cmake_minimum_required(VERSION 3.16)

# Define el proyecto, su nombre y el lenguaje principal (C++).
project(dynaRange CXX)

# --- Opciones y variables globales del compilador ---
# Establece el estándar C++ a C++17 y asegura que es requerido.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Añade definiciones globales para compatibilidad, especialmente en Windows.
# -DNOMINMAX es útil para evitar conflictos con min/max en <windows.h>
# -D_USE_MATH_DEFINES asegura que constantes como M_PI estén disponibles.
add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)

# --- Rutas y directorios ---
# Añade la carpeta /cmake local para módulos personalizados de CMake si los hubiera.
# (Actualmente no tienes, pero es buena práctica mantenerlo si se añaden en el futuro)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Directorio de salida para los ejecutables y recursos.
# CMake_BINARY_DIR es el directorio 'build' que creamos.
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
# Todas las librerías externas que el proyecto necesita.
# 'REQUIRED' significa que si no se encuentran, CMake falla.

find_package(PkgConfig REQUIRED)

# --- Librerías principales ---
pkg_check_modules(LIBRAW REQUIRED libraw)
find_package(OpenCV 4 REQUIRED)
find_package(Eigen3 3.3 REQUIRED)
find_package(CLI11 REQUIRED)

# Para Gettext, necesitamos una lógica condicional para el enlazado de 'intl'
find_package(Gettext REQUIRED)
# Define una variable para las librerías adicionales de Gettext.
# En Windows, añadiremos 'intl' aquí. En Linux, esta variable quedará vacía.
set(GETTEXT_EXTRA_LIBS)
if(WIN32)
    list(APPEND GETTEXT_EXTRA_LIBS intl)
endif()

# wxWidgets: se especifican los componentes para una detección robusta.
find_package(wxWidgets COMPONENTS core base html xrc qa net REQUIRED)

# Cairo: para capacidades gráficas.
pkg_check_modules(CAIRO REQUIRED cairo)


# =============================================================================
# 4. AGRUPACIÓN DE FICHEROS FUENTE
# =============================================================================
# Lista de ficheros fuente que son compartidos entre la versión CLI y la GUI.
# Esto evita duplicación y asegura que todos los componentes centrales se compilen.
set(CORE_SOURCES
    src/core/Analysis.cpp
    src/core/Arguments.cpp
    src/core/ChartProfile.cpp
    src/core/Engine.cpp
    src/core/engine/Initialization.cpp
    src/core/engine/Processing.cpp
    src/core/engine/Reporting.cpp
    src/core/graphics/Drawing.cpp
    src/core/graphics/Plotting.cpp
    src/core/Math.cpp
    src/core/RawFile.cpp
    src/core/ImageProcessing.cpp
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
# Crea el ejecutable 'dynaRangeGui'.
# 'WIN32' en Windows oculta la consola de comandos al ejecutar la GUI. Ignorado en Linux.
add_executable(dynaRangeGui WIN32
    src/DynaRangeGuiApp.cpp
    src/gui/DynaRangeBase.cpp
    src/gui/DynaRangeFrame.cpp
    ${CORE_SOURCES}
    # Fichero de recursos para el icono y metadatos en Windows.
    icono_noise.rc
)

# Directorios de inclusión para el ejecutable GUI.
target_include_directories(dynaRangeGui PRIVATE 
    ${OpenCV_INCLUDE_DIRS}
    ${LIBRAW_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${wxWidgets_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
)

# Definiciones de compilador específicas para wxWidgets.
# ${wxWidgets_DEFINITIONS} contiene flags como -D__WXGTK__ o -D__WXMSW__
# que son cruciales para que las cabeceras de wxWidgets se compilen correctamente.
target_compile_definitions(dynaRangeGui PRIVATE 
    ${wxWidgets_DEFINITIONS}
)

# Librerías con las que el ejecutable GUI debe enlazarse.
target_link_libraries(dynaRangeGui PRIVATE
    ${wxWidgets_LIBRARIES} # Contiene todas las librerías base de wxWidgets
    ${OpenCV_LIBS}
    ${LIBRAW_LIBRARIES}
    CLI11::CLI11
    ${GETTEXT_LIBRARIES}
    ${GETTEXT_EXTRA_LIBS} # Nuestra variable condicional para 'intl'
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
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION}) # Asumiendo que PROJECT_VERSION está definido en otro lugar o se definirá.
                                              # Si no, podrías usar una versión fija aquí, ej. "1.0.0"

# Instala los ejecutables en el directorio 'bin' del destino de instalación.
install(TARGETS rango dynaRangeGui DESTINATION bin)

# Incluye CPack para generar paquetes de instalación.
include(CPack)