# Entregas — trabajo de noviembre de 2026

Documentación y pruebas de los cambios hechos en la rama `feature/nikon-zero-black`.
El código está en los commits de la rama; aquí solo van las notas, los resultados de las
pruebas y los programas de medida usados.

| Carpeta | Qué se hizo | Commits |
|---|---|---|
| [2026-11-12_deteccion_esquinas](2026-11-12_deteccion_esquinas/) | Detección robusta de las 4 esquinas de la carta (círculos), probada con 120 RAW de la Sony a7 IV. Arreglos para compilar con OpenCV 5 / LibRaw 0.22. | `949f036`, `06c25d0` |
| [2026-11-12_negro_nikon](2026-11-12_negro_nikon/) | Nivel de negro en cámaras que lo recortan a 0 (Nikon D600/D610…): negro de los metadatos del RAW y nuevo estimador del ruido recortado. Medido con dark frames de una D610 (ISO 100 a Hi 2). | `ae4f550` |
| [2026-11-20_exportar_rejilla](2026-11-20_exportar_rejilla/) | Opción `-E` / `--export-grid` y casilla en la interfaz: carta enderezada con los valores RAW reales (TIFF 16 bits, sin interpolar) + CSV con la rejilla de parches. La interfaz deja negro y saturación en "auto". | `6a20830`, `5efd8b2` |

Orden de los commits en la rama (de más antiguo a más nuevo):

```
949f036  Build fixes for OpenCV 5 and LibRaw 0.22 (MSYS2 UCRT64)
06c25d0  Robust chart corner detection based on round marker blobs
ae4f550  Use black level 0 for cameras that clip black in-camera (Nikon)
6a20830  Add --export-grid (-E): corrected chart with untouched RAW values
5efd8b2  GUI: export-grid checkbox; black/saturation default to automatic
```

## Notas

- Los LEEME de cada carpeta mencionan una subcarpeta `codigo/` con parches (`git am …`).
  Esos parches no se suben porque el mismo código ya está en los commits de esta rama:
  basta con fusionar la rama.
- Del ejemplo de exportación solo se incluye el canal G1 (`iso6400_grid_G1.tif`); los otros
  tres canales se regeneran con `rango -E`.
- En los registros de las pruebas, la carpeta de salida aparece como `<carpeta de salida>`.

## Pendiente

- Serie de la Nikon D610 sobre la carta (ISO 100 a Hi 2) y toma quemada, para medir su
  rango dinámico y confirmar el detector de esquinas y la línea blanca del borde.
- Fallo de lectura de parches con monitor OLED (`docs/next_steps.txt`, punto 4).
- `rango.exe` se cae con rutas con acentos y guarda los resultados en Documentos aunque se
  use `-o`.
- Segunda app (visual) de muestras de ruido a partir de las imágenes exportadas.
