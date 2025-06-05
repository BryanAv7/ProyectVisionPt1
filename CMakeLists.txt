cmake_minimum_required(VERSION 3.15)
project(VisionApp VERSION 1.0 LANGUAGES CXX)

# 1. Estándar C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Activar AUTOUIC/AUTOMOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 3. Buscar Qt6 o Qt5
find_package(Qt6 COMPONENTS Widgets QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

# 4. Rutas ITK
set(ITK_DIR "/usr/local/itk/lib/cmake/ITK-6.0")
find_package(ITK REQUIRED COMPONENTS
    ITKCommon
    ITKStatistics
    ITKIOImageBase
    ITKThresholding
    ITKIONIFTI
    ITKImageIntensity
    ITKIOPNG
)
include(${ITK_USE_FILE})

# 5. Rutas OpenCV
set(OpenCV_DIR "/home/bryan/opencv/opencvi")
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui imgcodecs)

# 6. Incluir la carpeta raíz (para los headers .h)
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${ITK_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

# 7. Fuentes (fíjate en minúsculas exactas)
set(SRC_FILES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
)

# 8. Definir ejecutable Qt
if (QT_VERSION_MAJOR EQUAL 6)
    qt_add_executable(VisionApp
        MANUAL_FINALIZATION
        ${SRC_FILES}
    )
else()
    add_executable(VisionApp
        ${SRC_FILES}
    )
endif()

# 9. Enlazar Qt, ITK y OpenCV
if (QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(VisionApp
        PRIVATE Qt6::Widgets
                ${ITK_LIBRARIES}
                ${OpenCV_LIBS}
                dl
                pthread
                m
    )
else()
    target_link_libraries(VisionApp
        PRIVATE Qt5::Widgets
                ${ITK_LIBRARIES}
                ${OpenCV_LIBS}
                dl
                pthread
                m
    )
endif()

# 10. Carpeta de salida
set_target_properties(VisionApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 11. Finalizar para Qt6
if (QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(VisionApp)
endif()

