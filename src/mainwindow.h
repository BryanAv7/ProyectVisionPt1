// mainwindow.h

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QString>

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkExtractImageFilter.h>
#include <itkImageRegionConstIterator.h>

#include <opencv2/opencv.hpp>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void cargarImagen();
    void cargarMascara();
    void guardarTodo();
    void generarVideoCortes(const std::string& nombreArchivoVideo);
    void limpiarInterfaz();

private:
    Ui::MainWindow *ui;

    // Tipos ITK
    using Imagen3DFloat = itk::Image<float, 3>;
    using Imagen3DUChar = itk::Image<unsigned char, 3>;
    using Imagen2DFloat = itk::Image<float, 2>;
    using Imagen2DUChar = itk::Image<unsigned char, 2>;
    QString rutaImagenOriginal; // Variable auxiliar

    // Volúmenes cargados
    Imagen3DFloat::Pointer volumenOriginal;
    Imagen3DUChar::Pointer volumenMascara;

    int corteActual;
    int maxCorte;

    cv::Mat resultadoGlobal;  // Imagen resultante con bordes sobrepuestos

    // Funciones auxiliares
    template<typename TImage>
    typename TImage::Pointer cargarVolumen(const QString &ruta);

    template<typename Imagen3D, typename Imagen2D>
    typename Imagen2D::Pointer extraerCorte(typename Imagen3D::Pointer volumen, unsigned int z);

    cv::Mat convertirAGris(typename Imagen2DFloat::Pointer imagen2D);
    cv::Mat convertirAMascara(typename Imagen2DUChar::Pointer mascara2D);
    QImage matAQImage(const cv::Mat &mat);

    void calcularEstadisticas(typename Imagen2DFloat::Pointer corteOriginal,
                              typename Imagen2DUChar::Pointer corteMascara);

    void actualizarVisualizacionCorte();
    void procesarYMostrarCorte(typename Imagen2DFloat::Pointer corteOriginal,
                               typename Imagen2DUChar::Pointer corteMascara);

    std::string generarNombreUnico(const std::string& rutaBase, const std::string& extension);
};

#endif // MAINWINDOW_H
