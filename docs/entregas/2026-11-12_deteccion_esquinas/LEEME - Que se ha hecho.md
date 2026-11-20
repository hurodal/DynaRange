DynaRange — Mejora de la detección automática de esquinas
=========================================================

Fecha: 12/11/2026
Rama de git: feature/robust-corner-detection (creada a partir de main, commit ac80279 "Refact artifacts")

No se ha tocado nada de main ni de GitHub. Todo el trabajo está en una rama nueva aparte.


1. QUÉ PROBLEMA HABÍA
---------------------

El programa encuentra la carta en la foto buscando los 4 círculos blancos de las esquinas.
El método anterior dividía la imagen en 4 cuartos y, en cada uno, tomaba los píxeles más
brillantes y calculaba su posición media.

Problema: cualquier cosa brillante que no sea el círculo "tira" de la esquina hacia ella:
  - una lámpara o reflejo en la habitación,
  - el borde del monitor o el escritorio,
  - líneas blancas en el borde del sensor (caso de la Nikon D600),
  - los propios parches de la carta cuando la foto está muy subexpuesta.

Por eso con algunas cámaras o tomas la rejilla se colocaba mal y las mediciones salían mal.


2. QUÉ SE HA CAMBIADO
---------------------

Fichero modificado: src/core/graphics/detection/ChartCornerDetector.cpp
(El resto del programa no cambia; sigue llamando a la misma función.)

El nuevo detector:
  1. Suaviza ligeramente la imagen para eliminar ruido y píxeles calientes.
  2. Ignora una franja estrecha en todo el borde de la imagen (píxeles enmascarados,
     líneas blancas del borde, objetos cortados por el encuadre).
  3. Prueba varios niveles de brillo, para que un objeto más brillante que los círculos
     no los "tape".
  4. Solo acepta manchas REDONDAS y de tamaño razonable. Descarta parches (cuadrados),
     letras, líneas y bordes.
  5. Exige que cada mancha destaque claramente sobre el ruido del fondo.
  6. De todos los candidatos, elige los 4 (uno por cuadrante) que forman la carta más
     coherente: cuadrilátero convexo y grande, círculos de tamaño y brillo parecidos,
     lados opuestos de longitud parecida, cerca de las esquinas de la imagen.
  7. Calcula el centro de cada círculo con precisión de fracción de píxel (centroide
     ponderado por brillo), en lugar de la mediana de unos pocos píxeles.
  8. Si no encuentra los 4 círculos, avisa en el log y usa el método antiguo como
     respaldo, así que nunca funciona peor que antes.


3. CÓMO SE HA PROBADO
---------------------

Se han pasado los dos detectores (antiguo y nuevo) por los 120 RAW (.ARW, Sony a7 IV) de la
carpeta compartida "Compartida Guillermo-Hugo" y se han comparado las esquinas.
Resultado completo: resultados_120_RAW.txt (OLD = detector anterior, NEW = nuevo,
maxdiff = diferencia máxima en píxeles del canal G1).

Casos en los que el detector anterior FALLABA y el nuevo acierta
(ver carpeta "imagenes": ROJO = anterior, VERDE = nuevo):

  - _DSC8691 Fuga doble fuerte f4 ........ una esquina desplazada unos 130 px
  - Test convergencia a7IV 08 ............ dos esquinas en sitios equivocados
  - Test convergencia a7IV 16 ............ una lámpara arriba confundía una esquina
  - _DSC8740 SONY a7IV -9 EV ............. con -9 EV cogía las esquinas de los parches
  - _DSC8741 SONY a7IV -10 EV ............ ídem con -10 EV

Caso con objetos brillantes alrededor (equipos, cámara) en el que ambos aciertan:
  - _DSC9370 Test Fugas SONY a7IV

Resto de fotos con carta: los dos coinciden (diferencias de 0 a 14 px). Donde hay diferencia,
el nuevo queda más centrado en el círculo (ver "Ampliacion marcadores (precision del
centro).jpg": las cruces verdes caen en el centro; las rojas, desplazadas).

Dark frames y saturation frames (no tienen círculos): ahora el programa avisa de que no
encuentra marcadores en vez de inventarse unas esquinas. Estas tomas nunca se usan para
detectar la carta, así que esto no afecta al resultado.

Prueba completa del programa (rango.exe) con la carpeta
"2025-10-05 Test DR SONY a7IV carta OK" (dark frame + saturation frame + 7 ISOs):
detecta bien las 4 esquinas y el análisis termina con normalidad
(p. ej. ISO 100: RD 11,45 EV a 12 dB y 13,73 EV a 0 dB, normalización 0 Mpx).
Imagen: imagenes/Prueba completa 2025-10-05 - esquinas detectadas.jpg

PENDIENTE DE PROBAR: Nikon D600 (línea blanca vertical) y monitor OLED, porque esos RAW no
están en la carpeta compartida. Si nos pasáis algunos, se comprueban.
Los RAW de la OM-1 de la carpeta usan otra carta (sin círculos), así que no sirven para
esta prueba.


4. CONTENIDO DE ESTA CARPETA
----------------------------

  LEEME - Que se ha hecho.md ........ este documento
  resultados_120_RAW.txt ............ comparación detector anterior / nuevo en los 120 RAW
  imagenes/ ......................... capturas antes / después
  codigo/
    ChartCornerDetector.cpp ......... el fichero nuevo completo
    0001-...patch ................... commit 1: ajustes para compilar con OpenCV 5 / LibRaw 0.22
    0002-...patch ................... commit 2: el nuevo detector de esquinas
    DynaRange-deteccion-esquinas.bundle .. la rama de git completa (los 2 commits)

Sobre el commit 1: son tres cambios mínimos para que el proyecto compile con las versiones
actuales de MSYS2 (OpenCV 5 y LibRaw 0.22). No cambian el comportamiento del programa.


5. CÓMO INCORPORARLO AL REPOSITORIO (para Juanma)
-------------------------------------------------

Opción A — desde el bundle (trae la rama tal cual):
    git fetch "ruta/a/DynaRange-deteccion-esquinas.bundle" feature/robust-corner-detection:feature/robust-corner-detection
    git switch feature/robust-corner-detection

Opción B — aplicando los parches sobre main:
    git switch -c feature/robust-corner-detection
    git am "ruta/a/codigo/0001-*.patch" "ruta/a/codigo/0002-*.patch"

Opción C — a mano: sustituir src/core/graphics/detection/ChartCornerDetector.cpp por el
de la carpeta "codigo" y recompilar.


6. OTRAS COSAS DETECTADAS (NO CORREGIDAS)
-----------------------------------------

  - Con la opción -D (imágenes de depuración), algunas imágenes salen corruptas
    (debug_crop_area y debug_post_keystone): varios hilos escriben el mismo fichero a la vez.
  - En CornerDetectionHandler.cpp, para sensores con patrón BGGR se coge el canal R en vez
    del G1 (desplazamiento 1,1). Con marcadores blancos apenas se nota, pero es incorrecto.
  - El diagrama de flujo dice "spline cúbica", pero el código ajusta un polinomio
    (PolyFit, orden 3 por defecto) de EV en función de SNR (CurveCalculator.cpp).
  - La interfaz gráfica no llama al ejecutable de línea de comandos: ambos usan
    directamente el mismo código (src/core). Lo arreglado vale para los dos.
