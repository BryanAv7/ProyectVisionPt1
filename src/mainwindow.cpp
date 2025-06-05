// Librerías
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <filesystem>
#include <fstream>
#include <limits>
#include <cmath>

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <QScreen>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    corteActual(0),
    maxCorte(0),
    volumenOriginal(nullptr),
    volumenMascara(nullptr)
{
    ui->setupUi(this);

    // Centrar la ventana en la pantalla
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - this->width()) / 2;
    int y = (screenGeometry.height() - this->height()) / 2;
    this->move(x, y);

    // Conexión de botones
    connect(ui->btnLoadImage, &QPushButton::clicked, this, &MainWindow::cargarImagen);
    connect(ui->btnLoadMask,  &QPushButton::clicked, this, &MainWindow::cargarMascara);
    connect(ui->btnSaveAll,   &QPushButton::clicked, this, &MainWindow::guardarTodo);

    // Deshabilitamos el botón “Guardar todo” hasta tener ambos volúmenes
    ui->btnSaveAll->setEnabled(false);
}

MainWindow::~MainWindow() {
    delete ui;
}

// -------------------------------------------------------------
//           IMPLEMENTACIÓN DE FUNCIONES AUXILIARES
// -------------------------------------------------------------

template<typename TImage>
typename TImage::Pointer MainWindow::cargarVolumen(const QString &ruta) {
    using LectorTipo = itk::ImageFileReader<TImage>;
    auto lector = LectorTipo::New();
    lector->SetFileName(ruta.toStdString());
    lector->Update();
    return lector->GetOutput();
}

template<typename Imagen3D, typename Imagen2D>
typename Imagen2D::Pointer MainWindow::extraerCorte(typename Imagen3D::Pointer volumen, unsigned int z) {
    auto region3D = volumen->GetLargestPossibleRegion();
    auto size3D   = region3D.GetSize();
    if (z >= size3D[2]) {
        throw std::runtime_error("Índice de corte fuera de rango");
    }

    typename Imagen3D::IndexType inicio = region3D.GetIndex();
    inicio[2] = z;

    typename Imagen3D::SizeType tamañoCorte = size3D;
    tamañoCorte[2] = 0;

    typename Imagen3D::RegionType regionCorte;
    regionCorte.SetIndex(inicio);
    regionCorte.SetSize(tamañoCorte);

    using FiltroExtraer = itk::ExtractImageFilter<Imagen3D, Imagen2D>;
    auto extractor = FiltroExtraer::New();
    extractor->SetInput(volumen);
    extractor->SetExtractionRegion(regionCorte);
    extractor->SetDirectionCollapseToIdentity();
    extractor->Update();

    return extractor->GetOutput();
}

cv::Mat MainWindow::convertirAGris(typename Imagen2DFloat::Pointer img) {
    auto region = img->GetLargestPossibleRegion();
    int ancho  = region.GetSize()[0];
    int alto   = region.GetSize()[1];
    cv::Mat flotante(alto, ancho, CV_32FC1), grises;

    itk::ImageRegionConstIterator<Imagen2DFloat> it(img, region);
    for (int y = 0; y < alto; ++y) {
        for (int x = 0; x < ancho; ++x, ++it) {
            flotante.at<float>(y, x) = it.Get();
        }
    }

    double minVal, maxVal;
    cv::minMaxLoc(flotante, &minVal, &maxVal);
    flotante.convertTo(grises, CV_8UC1,
                       255.0 / (maxVal - minVal),
                       -minVal * 255.0 / (maxVal - minVal));
    return grises;
}

cv::Mat MainWindow::convertirAMascara(typename Imagen2DUChar::Pointer img) {
    auto region = img->GetLargestPossibleRegion();
    int ancho = region.GetSize()[0];
    int alto  = region.GetSize()[1];
    cv::Mat mascaraBinaria(alto, ancho, CV_8UC1);

    itk::ImageRegionConstIterator<Imagen2DUChar> it(img, region);
    for (int y = 0; y < alto; ++y) {
        for (int x = 0; x < ancho; ++x, ++it) {
            mascaraBinaria.at<uchar>(y, x) = it.Get() ? 255 : 0;
        }
    }
    return mascaraBinaria;
}

QImage MainWindow::matAQImage(const cv::Mat &mat) {
    switch (mat.type()) {
    case CV_8UC1:
        return QImage(mat.data, mat.cols, mat.rows,
                      static_cast<int>(mat.step),
                      QImage::Format_Grayscale8).copy();
    case CV_8UC3: {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows,
                      static_cast<int>(rgb.step),
                      QImage::Format_RGB888).copy();
    }
    default:
        return {};
    }
}

void MainWindow::calcularEstadisticas(typename Imagen2DFloat::Pointer corteOriginal,
                                      typename Imagen2DUChar::Pointer corteMascara)
{
    auto region = corteOriginal->GetLargestPossibleRegion();
    itk::ImageRegionConstIterator<Imagen2DFloat> itOrig(corteOriginal, region);
    itk::ImageRegionConstIterator<Imagen2DUChar> itMasc(corteMascara, region);

    double suma     = 0.0;
    double sumaSq   = 0.0;
    double minimo   = std::numeric_limits<double>::max();
    double maximo   = std::numeric_limits<double>::lowest();
    int    contador = 0;

    for (itOrig.GoToBegin(), itMasc.GoToBegin(); !itOrig.IsAtEnd(); ++itOrig, ++itMasc) {
        if (itMasc.Get() > 0) {
            double v = itOrig.Get();
            suma   += v;
            sumaSq += v * v;
            minimo  = std::min(minimo, v);
            maximo  = std::max(maximo, v);
            ++contador;
        }
    }

    if (contador == 0) {
        ui->txtStats->setPlainText("Corte sin región segmentada.");
        return;
    }

    double media   = suma / contador;
    double stddev  = std::sqrt((sumaSq / contador) - (media * media));

    QString texto;
    texto  = "Media (región segmentada): " + QString::number(media) + "\n";
    texto += "Desviación estándar: "    + QString::number(stddev) + "\n";
    texto += "Mínimo (segmentado): "    + QString::number(minimo) + "\n";
    texto += "Máximo (segmentado): "    + QString::number(maximo) + "\n";
    texto += "Píxeles segmentados: "     + QString::number(contador) + "\n";

    ui->txtStats->setPlainText(texto);
}

// -------------------------------------------------------------
//        PROCESAMIENTO Y ACTUALIZACIÓN DE CADA CORTE
// -------------------------------------------------------------
void MainWindow::procesarYMostrarCorte(typename Imagen2DFloat::Pointer corteOriginal,
                                       typename Imagen2DUChar::Pointer corteMascara)
{
    // 1) Convertir a escala de grises y máscara binaria
    cv::Mat gris       = convertirAGris(corteOriginal);
    cv::Mat mascaraBin = convertirAMascara(corteMascara);

    // 2) Mostrar originales en los QLabel
    QImage imgGris = matAQImage(gris);
    ui->lblOriginalSlice->setPixmap(
        QPixmap::fromImage(imgGris).scaled(ui->lblOriginalSlice->size(), Qt::KeepAspectRatio));

    QImage imgMascara = matAQImage(mascaraBin);
    ui->lblMaskSlice->setPixmap(
        QPixmap::fromImage(imgMascara).scaled(ui->lblMaskSlice->size(), Qt::KeepAspectRatio));

    // 3) Aplicar procesamiento:
    //    - Primero, enmascaramos la región de interés (ROI) para que solo hagamos operaciones dentro del tumor.
    cv::Mat grisEnRegion;
    gris.copyTo(grisEnRegion);
    grisEnRegion.setTo(0, mascaraBin == 0);

    // 3.1) Contraste adaptativo (CLAHE) dentro de la región del tumor
    cv::Mat claheResult;
    {
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
        clahe->apply(grisEnRegion, claheResult);
    }

    // 3.2) Filtrado de mediana (suavizado) para reducir ruido fino dentro de la ROI
    cv::Mat suavizado;
    cv::medianBlur(claheResult, suavizado, 5);

    // 3.3) Dilatación opcional para realzar bordes internos
    cv::Mat dilatado;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::dilate(suavizado, dilatado, kernel);

    // 3.4) Detección de bordes dentro de la ROI (Canny con umbrales más bajos)
    cv::Mat bordes;
    cv::Canny(dilatado, bordes, 30, 100);

    // 4) Superponer bordes en canal rojo sobre la imagen en escala de grises original
    cv::Mat colorResult;
    cv::cvtColor(gris, colorResult, cv::COLOR_GRAY2BGR);
    {
        std::vector<cv::Mat> canales;
        cv::split(colorResult, canales);
        // Aumentamos el canal rojo donde haya bordes
        canales[2] = cv::max(canales[2], bordes);
        cv::merge(canales, colorResult);
    }

    // 5) Crear un overlay rojo transparente para todo el interior del tumor
    {
        // 5.1) Construimos una imagen completamente roja (B=0, G=0, R=255) del mismo tamaño
        cv::Mat overlay(colorResult.size(), colorResult.type(), cv::Scalar(0, 0, 255));

        // 5.2) Mezclamos colorResult + overlay con alpha = 0.5
        cv::Mat fused;
        cv::addWeighted(colorResult, 1.0, overlay, 0.5, 0.0, fused);

        // 5.3) Copiamos solo las zonas de la máscara (región del tumor)
        fused.copyTo(colorResult, mascaraBin);
    }

    resultadoGlobal = colorResult;

    // Mostrar resultado final en QLabel
    QImage imgResult = matAQImage(resultadoGlobal);
    ui->lblResultSlice->setPixmap(
        QPixmap::fromImage(imgResult).scaled(ui->lblResultSlice->size(), Qt::KeepAspectRatio));

    // 6) Calcular y mostrar estadísticas solo en región segmentada
    calcularEstadisticas(corteOriginal, corteMascara);
}

void MainWindow::actualizarVisualizacionCorte()
{
    if (!volumenOriginal || !volumenMascara) return;

    try {
        auto corteOrig = extraerCorte<Imagen3DFloat, Imagen2DFloat>(volumenOriginal, corteActual);
        auto corteMasc = extraerCorte<Imagen3DUChar, Imagen2DUChar>(volumenMascara, corteActual);
        procesarYMostrarCorte(corteOrig, corteMasc);

        // Carpeta para la máscara
        std::string carpetaMascara = "/home/bryan/proyectoInter/Datoscsv/cortecsv/";

        // Carpeta para la imagen original
        std::string carpetaOriginal = "/home/bryan/proyectoInter/Datoscsv/originalcsv/";

        // Guardar CSV de la imagen original
        {
            std::ostringstream nombreArchivo;
            nombreArchivo << carpetaOriginal << "corte_" << corteActual << "_orig.csv";

            std::ofstream archivo(nombreArchivo.str());
            if (!archivo.is_open()) {
                throw std::runtime_error("No se pudo abrir el archivo CSV para escritura (original).");
            }

            Imagen2DFloat::RegionType region = corteOrig->GetLargestPossibleRegion();
            Imagen2DFloat::SizeType size = region.GetSize();

            for (unsigned int y = 0; y < size[1]; ++y) {
                for (unsigned int x = 0; x < size[0]; ++x) {
                    Imagen2DFloat::IndexType idx = {{x, y}};
                    float valor = corteOrig->GetPixel(idx);
                    archivo << valor;
                    if (x < size[0] - 1) archivo << ",";
                }
                archivo << "\n";
            }
            archivo.close();
        }

        // Guardar CSV de la máscara
        {
            std::ostringstream nombreArchivo;
            nombreArchivo << carpetaMascara << "corte_" << corteActual << "_mask.csv";

            std::ofstream archivo(nombreArchivo.str());
            if (!archivo.is_open()) {
                throw std::runtime_error("No se pudo abrir el archivo CSV para escritura (máscara).");
            }

            Imagen2DUChar::RegionType region = corteMasc->GetLargestPossibleRegion();
            Imagen2DUChar::SizeType size = region.GetSize();

            for (unsigned int y = 0; y < size[1]; ++y) {
                for (unsigned int x = 0; x < size[0]; ++x) {
                    Imagen2DUChar::IndexType idx = {{x, y}};
                    unsigned char valor = corteMasc->GetPixel(idx);
                    archivo << static_cast<int>(valor);
                    if (x < size[0] - 1) archivo << ",";
                }
                archivo << "\n";
            }
            archivo.close();
        }

    }
    catch (const std::exception &e) {
        QMessageBox::warning(this, "Error", e.what());
    }
}

// -------------------------------------------------------------
//                CARGA DE IMAGEN Y MÁSCARA
// -------------------------------------------------------------
void MainWindow::cargarImagen() {
    const QString directorio = "/home/bryan/proyectoInter/build/Desktop-Debug/bin";
    QString nombreArchivo = QFileDialog::getOpenFileName(
        this,
        "Seleccionar Imagen Original (.nii)",
        directorio,
        "NIfTI Files (*.nii *.nii.gz)");

    if (nombreArchivo.isEmpty()) return;

    volumenOriginal = cargarVolumen<Imagen3DFloat>(nombreArchivo);
    if (!volumenOriginal) {
        QMessageBox::critical(this, "Error", "No se pudo cargar la imagen original.");
        return;
    }

    if (volumenMascara) {
        auto tamaño = volumenOriginal->GetLargestPossibleRegion().GetSize();
        maxCorte = static_cast<int>(tamaño[2]) - 1;
        corteActual = maxCorte / 2;
        ui->btnSaveAll->setEnabled(true);
        actualizarVisualizacionCorte();
    }
}

void MainWindow::cargarMascara() {
    const QString directorio = "/home/bryan/proyectoInter/build/Desktop-Debug/bin";
    QString nombreArchivo = QFileDialog::getOpenFileName(
        this,
        "Seleccionar Máscara (.nii)",
        directorio,
        "NIfTI Files (*.nii *.nii.gz)");

    if (nombreArchivo.isEmpty()) return;

    volumenMascara = cargarVolumen<Imagen3DUChar>(nombreArchivo);
    if (!volumenMascara) {
        QMessageBox::critical(this, "Error", "No se pudo cargar la máscara.");
        return;
    }

    if (volumenOriginal) {
        auto tamañoImg  = volumenOriginal->GetLargestPossibleRegion().GetSize();
        auto tamañoMasc = volumenMascara->GetLargestPossibleRegion().GetSize();
        if (tamañoImg != tamañoMasc) {
            QMessageBox::critical(this, "Error", "Dimensiones de imagen y máscara no coinciden.");
            volumenMascara = nullptr;
            return;
        }

        maxCorte    = static_cast<int>(tamañoImg[2]) - 1;
        corteActual = maxCorte / 2;
        ui->btnSaveAll->setEnabled(true);
        actualizarVisualizacionCorte();

        // Generar video automáticamente al cargar ambos volúmenes
        generarVideoCortes();
    }
}

// -------------------------------------------------------------
//                 GUARDAR TODO (IMÁGENES Y ESTADÍSTICAS)
// -------------------------------------------------------------
void MainWindow::guardarTodo() {
    const std::string dirCortes     = "/home/bryan/proyectoInter/build/Desktop-Debug/bin/cortesegmentado";
    const std::string dirResultados = "/home/bryan/proyectoInter/build/Desktop-Debug/bin/resultados";
    std::filesystem::create_directories(dirCortes);
    std::filesystem::create_directories(dirResultados);

    // 1) Guardar corte original en PNG
    QString rutaOrig = QFileDialog::getSaveFileName(
        this,
        "Guardar Corte Original",
        QString::fromStdString(dirCortes + "/corte_original.png"),
        "PNG Files (*.png)");
    if (!rutaOrig.isEmpty()) {
        auto corteOrig = extraerCorte<Imagen3DFloat, Imagen2DFloat>(volumenOriginal, corteActual);
        cv::Mat gris = convertirAGris(corteOrig);
        cv::imwrite(rutaOrig.toStdString(), gris);
    }

    // 2) Guardar máscara del corte en PNG
    QString rutaMasc = QFileDialog::getSaveFileName(
        this,
        "Guardar Máscara del Corte",
        QString::fromStdString(dirCortes + "/corte_mascara.png"),
        "PNG Files (*.png)");
    if (!rutaMasc.isEmpty()) {
        auto corteMasc = extraerCorte<Imagen3DUChar, Imagen2DUChar>(volumenMascara, corteActual);
        cv::Mat masc = convertirAMascara(corteMasc);
        cv::imwrite(rutaMasc.toStdString(), masc);
    }

    // 3) Guardar imagen procesada con bordes y overlay en PNG
    QString rutaRes = QFileDialog::getSaveFileName(
        this,
        "Guardar Resultado (bordes + overlay)",
        QString::fromStdString(dirResultados + "/corte_resultado.png"),
        "PNG Files (*.png)");
    if (!rutaRes.isEmpty()) {
        cv::imwrite(rutaRes.toStdString(), resultadoGlobal);
    }

    // 4) Guardar estadísticas en TXT
    QString rutaStats = QFileDialog::getSaveFileName(
        this,
        "Guardar Estadísticas",
        QString::fromStdString(dirResultados + "/corte_estadisticas.txt"),
        "Text Files (*.txt)");
    if (!rutaStats.isEmpty()) {
        auto corteOrig = extraerCorte<Imagen3DFloat, Imagen2DFloat>(volumenOriginal, corteActual);
        auto corteMasc = extraerCorte<Imagen3DUChar, Imagen2DUChar>(volumenMascara, corteActual);

        auto region = corteOrig->GetLargestPossibleRegion();
        itk::ImageRegionConstIterator<Imagen2DFloat> itOrig(corteOrig, region);
        itk::ImageRegionConstIterator<Imagen2DUChar> itMasc(corteMasc, region);

        double suma   = 0.0, sumaSq = 0.0;
        double minV   = std::numeric_limits<double>::max();
        double maxV   = std::numeric_limits<double>::lowest();
        int cnt       = 0;

        for (itOrig.GoToBegin(), itMasc.GoToBegin(); !itOrig.IsAtEnd(); ++itOrig, ++itMasc) {
            if (itMasc.Get() > 0) {
                double v = itOrig.Get();
                suma   += v;
                sumaSq += v * v;
                minV    = std::min(minV, v);
                maxV    = std::max(maxV, v);
                cnt++;
            }
        }

        std::ofstream archivo(rutaStats.toStdString());
        if (!archivo.is_open()) {
            QMessageBox::warning(this, "Error", "No se pudo abrir el archivo para estadísticas.");
            return;
        }

        if (cnt > 0) {
            double media  = suma / cnt;
            double stddev = std::sqrt((sumaSq / cnt) - (media * media));

            archivo << "Media (región segmentada): " << media << "\n";
            archivo << "Desviación estándar: "       << stddev << "\n";
            archivo << "Mínimo (segmentado): "       << minV << "\n";
            archivo << "Máximo (segmentado): "       << maxV << "\n";
            archivo << "Píxeles segmentados: "       << cnt  << "\n";
        } else {
            archivo << "Corte sin región segmentada.\n";
        }

        archivo.close();
    }
}

// -------------------------------------------------------------
//                   GENERAR VIDEO DE CORTES
// -------------------------------------------------------------
void MainWindow::generarVideoCortes() {
    if (!volumenOriginal || !volumenMascara) {
        QMessageBox::warning(this, "Error", "Primero carga la imagen y la máscara.");
        return;
    }

    const std::string dirSalida  = "/home/bryan/proyectoInter/build/Desktop-Debug/bin";
    std::filesystem::create_directories(dirSalida);
    std::string nombreArchivo    = dirSalida + "/transicion.avi";

    // Obtener tamaño de un corte para configurar VideoWriter
    auto primerCorte = extraerCorte<Imagen3DFloat, Imagen2DFloat>(volumenOriginal, 0);
    auto region      = primerCorte->GetLargestPossibleRegion();
    int ancho  = region.GetSize()[0];
    int alto   = region.GetSize()[1];

    int fourcc = cv::VideoWriter::fourcc('M','J','P','G');
    double fps = 10.0;
    cv::VideoWriter writer(nombreArchivo, fourcc, fps, cv::Size(ancho, alto));

    if (!writer.isOpened()) {
        QMessageBox::critical(this, "Error", "No se pudo crear el archivo de video.");
        return;
    }

    // Recorrer cada corte y escribir en el video
    for (unsigned int z = 0; z <= static_cast<unsigned int>(maxCorte); ++z) {
        auto corteOrig = extraerCorte<Imagen3DFloat, Imagen2DFloat>(volumenOriginal, z);
        auto corteMasc = extraerCorte<Imagen3DUChar, Imagen2DUChar>(volumenMascara, z);

        // Mismos pasos de procesamiento de bordes y overlay que en procesarYMostrarCorte
        cv::Mat gris       = convertirAGris(corteOrig);
        cv::Mat mascaraBin = convertirAMascara(corteMasc);

        cv::Mat grisEnRegion;
        gris.copyTo(grisEnRegion);
        grisEnRegion.setTo(0, mascaraBin == 0);

        cv::Mat claheResult;
        {
            cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
            clahe->apply(grisEnRegion, claheResult);
        }

        cv::Mat suavizado;
        cv::medianBlur(claheResult, suavizado, 5);

        cv::Mat dilatado;
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::dilate(suavizado, dilatado, kernel);

        cv::Mat bordes;
        cv::Canny(dilatado, bordes, 30, 100);

        cv::Mat colorResult;
        cv::cvtColor(gris, colorResult, cv::COLOR_GRAY2BGR);
        {
            std::vector<cv::Mat> canales;
            cv::split(colorResult, canales);
            canales[2] = cv::max(canales[2], bordes);
            cv::merge(canales, colorResult);
        }

        {
            cv::Mat overlay(colorResult.size(), colorResult.type(), cv::Scalar(0, 0, 255));
            cv::Mat fused;
            cv::addWeighted(colorResult, 1.0, overlay, 0.5, 0.0, fused);
            fused.copyTo(colorResult, mascaraBin);
        }

        writer.write(colorResult);
    }

    writer.release();
    QMessageBox::information(this, "Video generado",
                             QString("Video guardado en:\n%1/transicion.avi")
                                 .arg(QString::fromStdString(dirSalida)));
}
