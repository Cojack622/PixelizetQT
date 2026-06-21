#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "PixelizeWorker.h"

#include <QFileDialog>
#include <QDebug>


QImage CreateQImage(uchar** imgData, uint width, uint height, const char* imgFormat) {
    return QImage(*imgData, width, height, width*3, QImage::Format_BGR888);
}

void ChangeDisplayImage(uchar** imgData, uint width, uint height, const char* imgFormat) {

    auto img = CreateQImage(imgData, width, height, imgFormat);
    //img.save("tempOut.png");
    QPixmap pxMap = QPixmap::fromImage(img);


    MainWindowInstance->ui->PixelImageDisplay->setPixmap(pxMap);
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    if(MainWindowInstance == nullptr){
        MainWindowInstance = this;
    }
    
    Pixelizer::NewSuperPixelOutputSubrscribe(ChangeDisplayImage);

    // connect(ui->ImageUpload, SIGNAL(clicked()), this, SLOT(on_ImageUpload_clicked()));
    // connect(ui->btnOk, SIGNAL(clicked()),this,SLOT(on_btnOk_Clicked()));
    // connect(ui->btnCancel, SIGNAL(clicked()),this,SLOT(on_btnCancel_Clicked()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_ImageUpload_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "", tr("Image Files (*.png *.jpg *.bmp)"));    
    if (!fileName.isEmpty()) {
        _pixelizationSettings.inputImagePath = fileName;

        QPixmap pxMap = QPixmap(fileName);
        pxMap = pxMap.scaled(ui->PixelImageDisplay->size(), Qt::KeepAspectRatio);
        ui->PixelImageDisplay->setPixmap(pxMap);
    }
}

void MainWindow::on_StartPixelizet_clicked() {
    //Add check/Dialogue box for if image clicked here

    // Pixelizer::PixelizationInputSettings input;

    // input.scale = ui->OutputSizeSpinner->value();
    // input.paletteSize = ui->PaletteSizeSpinner->value();
    // input.inputFilePath = _pixelizationSettings.inputImagePath.toStdString();

    // Pixelizer::PixelizeImage(input);
    
    if(_isPixelizing) {
        return;
    }

    _isPixelizing = true;

    auto *worker = new PixelizeWorker();

    worker->input.scale = ui->OutputSizeSpinner->value();
    worker->input.paletteSize = ui->PaletteSizeSpinner->value();
    worker->input.inputFilePath = _pixelizationSettings.inputImagePath.toStdString();

    worker->moveToThread(&pixelizationThread);
    
    // Delete the worker when the thread is finished
    connect(&pixelizationThread, &QThread::finished, worker, &QObject::deleteLater);
    // Start Worker on thread start
    connect(&pixelizationThread, &QThread::started, worker, &PixelizeWorker::process);
    // Connect the worker's finished signal to the main window's slot
    connect(worker, &PixelizeWorker::finished, this, &MainWindow::on_PixelizationFinished);

    pixelizationThread.start();
}

void MainWindow::on_PixelizationFinished(const Pixelizer::PixelizationOutput &result) {
    _isPixelizing = false;
    _finalImage.hasFinal = result.pixelizationSuccess;
    if(result.pixelizationSuccess) {
        //Create non Const copy 
        cv::Mat outputCopy = result.output;
        _finalImage.finalImage = CreateQImage(&outputCopy.data, outputCopy.cols, outputCopy.rows, "RGB888");
    }
}