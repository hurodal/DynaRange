DynaRange — Corrección del nivel de negro en cámaras que lo recortan a 0 (Nikon)
================================================================================

Fecha: 12/11/2026
Rama de git: feature/nikon-zero-black (creada a partir de feature/robust-corner-detection)
Estado: commit ae4f550 en la rama. No se ha tocado main ni GitHub.
Para incorporarlo: git am "codigo/0001-Use-black-level-0-for-cameras-that-clip-black-in-cam.patch"
(sobre la rama feature/robust-corner-detection).


1. QUÉ PROBLEMA HABÍA
---------------------

Algunas Nikon (D600, D610, D800, D7100...) restan el nivel de negro dentro de la cámara
y guardan en el NEF todo lo que queda por debajo como 0. Medido con una Nikon D610
(dark frames de ISO 100 a Hi 2, ver carpeta "pruebas"):

  - Nivel de negro en los metadatos del NEF: 0, en todos los ISO.
  - Entre el 50 % y el 78 % de los píxeles de un dark frame valen exactamente 0.

DynaRange fallaba de tres maneras:

  a) Sin dark frame: buscaba el valor mínimo de la imagen (0) y, como es <= 1, usaba el
     valor por defecto 256. Restar 256 a una cámara con negro 0 vuelve negativas todas
     las sombras de más de ~6 EV por debajo de la saturación, que se descartan.
  b) Con dark frame (-b): tomaba como negro la media del dark frame (0,25 a ISO 100,
     16 a ISO 6400, 61 a Hi 2), que está sesgada hacia arriba por el recorte.
  c) La reconstrucción de los parches recortados (normal truncada) solo se activaba con
     negro exactamente 0,0, cosa que no pasaba nunca en a) ni en b). Y además el
     estimador (descenso de gradiente) no funcionaba: a ISO 100 diverge a valores
     absurdos y en el resto se queda casi en el punto de partida (ver
     "D610_estimador_antiguo_vs_nuevo.txt").


2. QUÉ SE HA CAMBIADO
---------------------

  - CalibrationHandler.cpp: se lee el nivel de negro de los metadatos del RAW.
      * Si es 0 -> negro = 0 (aunque se dé un dark frame, que se ignora para el negro
        y se avisa en el log). Solo un -B numérico explícito (sin -b) lo sobrescribe.
      * Si no hay dark frame ni -B -> se usa el negro de los metadatos (Sony: 512).
        La heurística antigua de la potencia de 2 queda solo como último recurso.
      * Con dark frame en cámaras con negro > 0 -> igual que antes (media del dark frame).
  - RawMetadataExtractor.cpp: el negro de metadatos se calcula bien
    (black + media de cblack[0..3] + media del patrón cblack[6..]); antes promediaba
    también las dimensiones del patrón (cblack[4], cblack[5]). Devuelve double.
  - TruncatedNormalEstimator.cpp + Constants.hpp: nuevo estimador EM de la normal
    censurada (sin tasas de aprendizaje, no depende de la escala). Los estimadores
    antiguos siguen en el código pero ya no se usan (USE_EM_ESTIMATOR = true).
  - ImageAnalyzer.cpp: el punto de corte es medio escalón de cuantización (mitad del
    menor valor positivo del parche). En la D610, ISO 3200 y 6400 son ganancia digital
    x2 y x4 (los RAW solo usan valores pares / múltiplos de 4), y un 0 significa
    "menor que 1" y "menor que 2" respectivamente.


3. CÓMO SE HA PROBADO
---------------------

  - Prueba sintética (ruido gaussiano, valores enteros, recorte a 0, ganancia x1/x2/x4):
    el estimador acierta con error de ±1 % cuando el ruido es >= 3 unidades RAW, +3 %
    con 1,3 unidades. Con ruido < 1 unidad (D610 a ISO 100 en negro) sobreestima
    (~+34 %): ahí la cuantización domina y no hay información para más.
    Ver "estimador_prueba_sintetica.txt".
  - Dark frames reales de la D610: el estimador nuevo da negro ~0 (como debe) y un ruido
    coherente con el de las columnas enmascaradas del sensor. Algo por encima (~10 %)
    del ajuste por cuantiles porque el ruido real tiene la cola algo más pesada que una
    gaussiana.
  - Sony a7 IV (2025-10-05, ISO 100/800/6400): tabla de rango dinámico IDÉNTICA antes y
    después, con dark/saturation frame y sin ellos. Ver "Sony_*_antes/despues.txt".
  - rango.exe con NEF de la D610: elige negro 0 sin dark frame, con dark frame (-b) y
    con -B 0.

PENDIENTE: prueba completa con una serie de la D610 sobre la carta (ISO 100 a Hi 2) y un
saturation frame. Sin carta no se puede medir aún el rango dinámico final.


4. OTRAS COSAS OBSERVADAS EN LA D610 (NO CORREGIDAS)
----------------------------------------------------

  - Desde ISO 1600 la cámara solo multiplica digitalmente (3200 = x2, 6400 = x4, Hi =
    factores no enteros). Por encima de 1600 el RD debería caer exactamente 1 EV por paso.
  - Las Hi salen con ISO 8063 / 10159 / 12800 / 25600 en vez de "Hi 0,3 / 0,7 / 1 / 2".
  - El margen derecho del sensor (columnas 6034–6079, fuera del área activa) tiene
    columnas muy brillantes (6042–6047): probablemente la "línea blanca vertical".
    LibRaw 0.22 la recorta y DynaRange solo usa el área activa.
  - rango.exe se cae (wstring_convert) si la ruta de los ficheros tiene acentos
    (p. ej. "Rango Dinámico"). Y deja los resultados en Documentos aunque se use -o.


5. CONTENIDO DE ESTA CARPETA
----------------------------

  codigo/negro-nikon.diff ... todos los cambios (git apply sobre la rama de las esquinas)
  pruebas/ .................. resultados y los programas de medida usados (C++ + LibRaw)
