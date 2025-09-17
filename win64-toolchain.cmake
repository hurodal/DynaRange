# win64-toolchain.cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compiladores MinGW-w64 para Windows 64 bits
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

# Ruta donde se instalan las librerías de MinGW-w64 (ajusta si es necesario)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Solo busca bibliotecas e headers en el entorno MinGW, no en el sistema Linux
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 💥 CLAVE: Enlaza TODO estáticamente → ¡Monobloque!
set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")

# Desactiva cualquier flag de 32 bits (¡tu código actual lo fuerza en WIN32!)
# Esto reemplaza el `-m32` que tienes en tu CMakeLists.txt
set(CMAKE_C_FLAGS "")
set(CMAKE_CXX_FLAGS "")