#!/bin/bash

# Función recursiva para procesar directorios
procesar_directorio() {
    local dir="$1"
    local ruta_relativa="$2"  # Ruta relativa desde el directorio base
    local output_base="$3"    # Directorio base de salida (carpeta "sources")
    local full_file="$4"      # Ruta al archivo full_source.txt

    # Verificar si el directorio existe
    if [ ! -d "$dir" ]; then
        echo "Error: El directorio '$dir' no existe." >&2
        return 1
    fi

    echo "Procesando directorio: $dir"

    # Cambiar al directorio actual (solo para leer)
    cd "$dir" || return 1

    # Construir el nombre del archivo de salida individual
    if [ -z "$ruta_relativa" ]; then
        nombre_archivo="sources_$(basename "$dir").txt"
    else
        ruta_normalizada="${ruta_relativa//\//_}"
        nombre_archivo="sources_${ruta_normalizada}.txt"
    fi

    ruta_salida="$output_base/$nombre_archivo"

    # Obtener archivos .hpp y .cpp
    shopt -s nullglob
    archivos=(*.hpp *.cpp)
    shopt -u nullglob

    # SOLO proceder si hay al menos un archivo válido
    hay_archivos=false
    for archivo in "${archivos[@]}"; do
        if [ -f "$archivo" ]; then
            hay_archivos=true
            break
        fi
    done

    if [ "$hay_archivos" = false ]; then
        echo "⚠️  El directorio '$dir' no contiene archivos .hpp ni .cpp. Omitiendo..."
        # Procesar subdirectorios igualmente
        for subdir in */; do
            [ -d "$subdir" ] && [ "$subdir" != "./" ] && [ "$subdir" != "../" ] || continue
            subdir_name="${subdir%/}"
            if [ -z "$ruta_relativa" ]; then
                nueva_ruta="$subdir_name"
            else
                nueva_ruta="$ruta_relativa/$subdir_name"
            fi
            procesar_directorio "$subdir_name" "$nueva_ruta" "$output_base" "$full_file"
            cd ..
        done
        return 0
    fi

    # Crear directorio de salida si no existe
    mkdir -p "$(dirname "$ruta_salida")" 2>/dev/null || true

    # Crear/limpiar el archivo individual
    > "$ruta_salida"

    # Bandera para separadores (para archivo individual)
    primero_individual=true
    # Bandera para saber si es el primer archivo global (para full_source.txt)
    local es_primer_archivo_global=false
    if [ ! -s "$full_file" ]; then
        es_primer_archivo_global=true
    fi

    for archivo in "${archivos[@]}"; do
        [ ! -f "$archivo" ] && continue

        # Escribir en archivo individual
        if [ "$primero_individual" = false ]; then
            echo "" >> "$ruta_salida"
            echo "" >> "$ruta_salida"
            echo "=======================" >> "$ruta_salida"
            echo "" >> "$ruta_salida"
        fi
        cat "$archivo" >> "$ruta_salida"
        primero_individual=false

        # Escribir en archivo global full_source.txt
        if [ "$es_primer_archivo_global" = false ]; then
            echo "" >> "$full_file"
            echo "" >> "$full_file"
            echo "=======================" >> "$full_file"
            echo "" >> "$full_file"
        else
            es_primer_archivo_global=false
        fi
        cat "$archivo" >> "$full_file"
    done

    echo "✅ Creado: $ruta_salida"

    # Procesar subdirectorios
    for subdir in */; do
        [ -d "$subdir" ] && [ "$subdir" != "./" ] && [ "$subdir" != "../" ] || continue
        subdir_name="${subdir%/}"
        if [ -z "$ruta_relativa" ]; then
            nueva_ruta="$subdir_name"
        else
            nueva_ruta="$ruta_relativa/$subdir_name"
        fi
        procesar_directorio "$subdir_name" "$nueva_ruta" "$output_base" "$full_file"
        cd ..
    done
}

# Verificar argumento
if [ $# -eq 0 ]; then
    echo "Uso: $0 <directorio>"
    echo "Ejemplo: $0 src"
    exit 1
fi

DIRECTORIO_BASE="$1"

if [ ! -d "$DIRECTORIO_BASE" ]; then
    echo "Error: El directorio '$DIRECTORIO_BASE' no existe."
    exit 1
fi

DIRECTORIO_ACTUAL="$(pwd)"
mkdir -p "$DIRECTORIO_ACTUAL/sources"

# Definir ruta del archivo full_source.txt
FULL_SOURCE_FILE="$DIRECTORIO_ACTUAL/sources/full_source.txt"

# Limpiar o crear el archivo full_source.txt
> "$FULL_SOURCE_FILE"

# Iniciar procesamiento recursivo, pasando la ruta del full_source.txt
procesar_directorio "$DIRECTORIO_BASE" "" "$DIRECTORIO_ACTUAL/sources" "$FULL_SOURCE_FILE"

cd "$DIRECTORIO_ACTUAL" || exit 1

echo "✅ Proceso completado. Archivos generados en: $DIRECTORIO_ACTUAL/sources"
echo "📄 Archivo completo: $FULL_SOURCE_FILE"
