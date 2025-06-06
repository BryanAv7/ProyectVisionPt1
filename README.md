# ProyectoVisionPt1

Nombre: Bryan Avila
Carrera: Computación

# Descripción

Este es un proyecto de visión computacional desarrollado en C++ utilizando Qt, ITK y OpenCV. Su objetivo principal es cargar volúmenes médicos (en formato NIfTI), visualizar cortes axiales, segmentar tumores, calcular estadísticas y generar videos del proceso.

## Tecnologías utilizadas

- **Qt5**: Para la interfaz gráfica de usuario (GUI).
- **ITK**: Para la lectura y procesamiento de volúmenes médicos 3D.
- **OpenCV**: Para operaciones de mejora visual, detección de bordes y procesamiento en 2D.
- **CMake**: Para la configuración y compilación del proyecto.

## Funcionalidades

- Cargar volúmenes médicos (`*.nii`) de imagen y máscara.
- Visualizar cortes axiales con sobreposición de la segmentación.
- Procesamiento de imagen 2D (CLAHE, Canny, Laplaciano).
- Generación de archivos CSV por corte con datos originales y de máscara.
- Cálculo de estadísticas de la región segmentada.
- Guardado de imágenes procesadas.
- Creación de video mostrando todos los cortes axiales procesados.

## Estructura del proyecto

\`\`\`
proyectoInter/
├── build/                  # Carpeta de compilación
├── codePython/             # Scripts Python (generar histogramas)
├── Datoscsv/               # CSVs generados por corte (original y máscara)
├── ImagenesTesteo/         # Imágenes NIfTI de entrada para pruebas
├── Resultados/             # Imágenes procesadas guardadas
├── Videogenerado/          # Videos generados desde los cortes
├── src/                    # Código fuente en C++
│   ├── main.cpp
│   └── mainwindow.h / .cpp
├── CMakeLists.txt
└── README.md
\`\`\`

## Cómo compilar

Desde la raíz del proyecto:

\`\`\`bash
mkdir build
cd build
cmake ..
make
\`\`\`

El ejecutable será generado (en este caso \`VisionApp\` donde lo tengo definido en mi \`CMakeLists.txt\`). Luego puedes ejecutarlo con:

\`\`\`bash
./VisionApp
\`\`\`

> Recuerda: Asegúrate de tener instaladas las dependencias necesarias: Qt5, OpenCV, ITK y CMake.

## Uso general

1. Abre la aplicación.
2. Usa el botón **"Cargar Imagen"** para seleccionar un archivo NIfTI de imagen.
3. Usa **"Cargar Máscara"** para seleccionar el volumen de segmentación correspondiente.
4. Visualización de los cortes en la interfaz.
5. Presiona **"Guardar Todo"** para generar:
   - Imágenes procesadas (en \`Resultados/\`).
   - Archivos CSV de los cortes (en \`Datoscsv/\`).
   - Video generado de todos los cortes (en \`Videogenerado/\`).
6. Para generar el histograma, debemos crear un entorno virtual de Python y ejecutar el siguiente archivo:
   - codePython/



