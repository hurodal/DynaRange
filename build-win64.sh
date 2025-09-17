#!/bin/bash

# Asegúrate de tener MinGW-w64 instalado
if ! command -v x86_64-w64-mingw32-g++.exe &> /dev/null; then
    echo "❌ Error: MinGW-w64 no está instalado. Instálalo con:"
    echo "   sudo apt install g++-mingw-w64-x86-64"
    exit 1
fi

# Directorios
BUILD_DIR="build-win64"
SOURCE_DIR="."

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configura CMake con toolchain estático
cmake \
  -DCMAKE_TOOLCHAIN_FILE="$SOURCE_DIR/win64-toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXE_LINKER_FLAGS="-static" \
  "$SOURCE_DIR"

# Compila
make -j$(nproc)

# Copia el ejecutable a una carpeta de distribución
DIST_DIR="../dist-win64"
mkdir -p "$DIST_DIR"
cp rango.exe dynaRangeGui.exe "$DIST_DIR/"

echo "✅ ¡Listo! Programa compilado y empaquetado en: $DIST_DIR/rango.exe y $DIST_DIR/dynaRangeGui.exe "
echo "   Este archivo funciona en cualquier Windows 64 bits sin instalar nada."