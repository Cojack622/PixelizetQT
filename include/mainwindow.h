#pragma once

#include <QMainWindow>
#include <Pixelizer.hpp>
#include <QThread>
#include <string>


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE



void ChangeDisplayImage(uchar** imgData, uint width, uint height, const char* imgFormat);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


    Ui::MainWindow *ui;

    static MainWindow* WindowMainInstance;

private slots:
    void on_ImageUpload_clicked();
    void on_StartPixelizet_clicked();


    void on_PixelizationFinished(const Pixelizer::PixelizationOutput &);

signals:
    void operate();

private:

    QThread pixelizationThread;
    // struct InputImageContext {
    //     QString inputImagePath;
    // };

    bool _isPixelizing = false;

    struct PixelizeSettings {
        QString inputImagePath;

        unsigned int paletteSize;
        unsigned int outputScale;
    } _pixelizationSettings;

    struct FinalImage {
        QImage finalImage;
        bool hasFinal;
    } _finalImage;

};

static MainWindow* MainWindowInstance;