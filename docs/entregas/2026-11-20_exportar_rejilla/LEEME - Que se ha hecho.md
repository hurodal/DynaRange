DynaRange — Exportar la carta corregida con los valores RAW reales
==================================================================

Fecha: 20/11/2026
Rama de git: feature/nikon-zero-black, commits 6a20830 (exportación, línea de comandos) y
5efd8b2 (interfaz gráfica). Van encima de la corrección del negro de Nikon y de la
detección de esquinas. No se ha tocado main ni GitHub.
Para incorporarlo, en este orden:
    git am "codigo/0001-Add-export-grid-E-corrected-chart-with-untouched-RAW.patch"
    git am "codigo/0002-GUI-export-grid-checkbox-black-saturation-default-to.patch"


1. QUÉ HACE
-----------

Nueva opción de línea de comandos:

    rango -E ...        (o --export-grid)

Para cada RAW analizado y para cada canal (R, G1, G2, B) guarda:

  <nombre del RAW>_grid_<canal>.tif
      La carta ya enderezada y recortada, exactamente igual que la que mide DynaRange,
      pero con los VALORES RAW ORIGINALES:
        - TIFF de 16 bits, un canal, un píxel por píxel del canal Bayer (media resolución).
        - Enderezado por vecino más próximo: sin interpolación (ni bilineal ni bicúbica).
        - Sin restar el negro ni escalar: cada píxel es un valor tal cual sale del sensor.

  <nombre del RAW>_grid_patches.csv
      Cabecera con el fichero, ISO, negro, saturación y patch_ratio, y una fila por canal
      y parche (A1 … D6: filas A-D de arriba abajo, columnas 1-6) con:
        x1, y1, x2, y2 ... rectángulo EXACTO que usa DynaRange para medir ese parche
                           (x2 e y2 excluidos), en coordenadas del TIFF
        pixels, mean_raw, std_raw ........... estadística simple de esos píxeles
        signal_norm, noise_norm, snr_db ..... lo mismo normalizado con negro y saturación

  Así se puede abrir el TIFF en cualquier programa (ImageJ/Fiji, Photoshop, Python, R...),
  medir un parche a mano y comparar con DynaRange.

  Nota: el TIFF tiene valores RAW (p. ej. 500–3.600 sobre 0–65.535), así que en un visor
  normal se verá casi negro. Para verlo hay que estirar niveles; para medir no hace falta.
  Nota 2: mean/std del CSV son estadística simple. En cámaras con el negro recortado a 0
  (Nikon) DynaRange además reconstruye el ruido recortado; el CSV no.


2. CÓMO SE HA PROBADO (Sony a7 IV, carpeta 2025-10-05)
------------------------------------------------------

  - Con esquinas fijas (-x) se ha reproducido el enderezado por separado (Python) y se ha
    comparado píxel a píxel con el RAW leído directamente con LibRaw:
    0 píxeles distintos de 4.043.535 en cada canal, a ISO 100 y a ISO 6400.
    Ver "pruebas/verificacion_pixel_a_pixel.txt".
  - Las medias y desviaciones del CSV coinciden con las recalculadas desde el TIFF
    (diferencia máxima 5e-7).
  - La tabla de rango dinámico es idéntica con y sin -E.
  - En "ejemplo_Sony_ISO6400" están los 4 TIFF y el CSV de una toma real.


3. PENDIENTE
------------

  - HECHO (commit 5efd8b2): casilla "Export grid (RAW)" en la interfaz gráfica, junto a
    "Full debug", con ayuda emergente. Añadida en dynarangeUI.fbp (wxFormBuilder) y en el
    código generado (DynaRangeBase.cpp/.h), así que no se pierde al regenerar. Al marcarla,
    el comando equivalente muestra --export-grid.
  - CORREGIDO (mismo commit): los campos "Dark Value" y "Sat. Value" venían rellenos con
    256.0 y 4095.0, así que la interfaz mandaba siempre -B 256 -S 4095 y nunca estimaba
    negro ni saturación (mal para la Sony, 512/16383, y anulaba el negro 0 de Nikon).
    Ahora empiezan vacíos con el texto "auto" y el programa los lee del RAW. Si se escribe
    un valor, se usa ese. El aviso de "fichero saturado" al cargar RAW también usa el
    blanco de cada fichero cuando el campo está en auto.
  - Probar con la serie de la D610 sobre la carta.
  - Los ficheros salen en la carpeta de resultados del programa (Documentos), igual que el
    resto de salidas.
