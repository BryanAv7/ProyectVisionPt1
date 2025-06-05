# ProyectoVisionPt1

Este es un proyecto de visión computacional desarrollado en C++ con Qt, ITK y OpenCV. Permite cargar volúmenes médicos, visualizar cortes axiales, segmentar tumores y generar videos con estadísticas.

## Tecnologías utilizadas

- Qt5 (interfaz gráfica)
- ITK (procesamiento de imágenes médicas)
- OpenCV (visualización y operaciones con imágenes)
- CMake (gestión del proyecto)

## Estructura del proyecto

- `src/`: Código fuente
- `ImagenesTesteo/`: Imágenes y volúmenes de prueba
- `build/`: Carpeta de compilación

## Cómo compilar

```bash
mkdir build
cd build
cmake ..
make
./bin/VisionApp
