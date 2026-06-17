/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout;
    QGridLayout *gridLayout;
    QPushButton *ImageUpload;
    QPushButton *ImageDownload;
    QPushButton *StartPixelizet;
    QVBoxLayout *verticalLayout_3;
    QLabel *PixelImageDisplay;
    QSplitter *ImageSize;
    QSpinBox *OutputSizeSpinner;
    QLabel *ImgSizeLabel;
    QVBoxLayout *verticalLayout;
    QSplitter *PaletteSize;
    QSpinBox *PaletteSizeSpinner;
    QLabel *label;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(852, 608);
        MainWindow->setMouseTracking(false);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(centralwidget->sizePolicy().hasHeightForWidth());
        centralwidget->setSizePolicy(sizePolicy);
        verticalLayout_2 = new QVBoxLayout(centralwidget);
        verticalLayout_2->setObjectName("verticalLayout_2");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        gridLayout->setSizeConstraint(QLayout::SizeConstraint::SetDefaultConstraint);
        ImageUpload = new QPushButton(centralwidget);
        ImageUpload->setObjectName("ImageUpload");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(ImageUpload->sizePolicy().hasHeightForWidth());
        ImageUpload->setSizePolicy(sizePolicy1);
        ImageUpload->setMinimumSize(QSize(0, 100));

        gridLayout->addWidget(ImageUpload, 1, 0, 1, 1, Qt::AlignmentFlag::AlignVCenter);

        ImageDownload = new QPushButton(centralwidget);
        ImageDownload->setObjectName("ImageDownload");
        sizePolicy1.setHeightForWidth(ImageDownload->sizePolicy().hasHeightForWidth());
        ImageDownload->setSizePolicy(sizePolicy1);
        ImageDownload->setMinimumSize(QSize(199, 100));

        gridLayout->addWidget(ImageDownload, 1, 1, 1, 1, Qt::AlignmentFlag::AlignVCenter);

        StartPixelizet = new QPushButton(centralwidget);
        StartPixelizet->setObjectName("StartPixelizet");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(2);
        sizePolicy2.setHeightForWidth(StartPixelizet->sizePolicy().hasHeightForWidth());
        StartPixelizet->setSizePolicy(sizePolicy2);
        StartPixelizet->setMinimumSize(QSize(200, 200));
        QFont font;
        font.setFamilies({QString::fromUtf8("Yu Gothic Light")});
        font.setPointSize(48);
        font.setBold(true);
        font.setStyleStrategy(QFont::NoAntialias);
        StartPixelizet->setFont(font);

        gridLayout->addWidget(StartPixelizet, 0, 0, 1, 2, Qt::AlignmentFlag::AlignVCenter);


        horizontalLayout->addLayout(gridLayout);

        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName("verticalLayout_3");
        PixelImageDisplay = new QLabel(centralwidget);
        PixelImageDisplay->setObjectName("PixelImageDisplay");
        QSizePolicy sizePolicy3(QSizePolicy::Policy::Ignored, QSizePolicy::Policy::Ignored);
        sizePolicy3.setHorizontalStretch(2);
        sizePolicy3.setVerticalStretch(3);
        sizePolicy3.setHeightForWidth(PixelImageDisplay->sizePolicy().hasHeightForWidth());
        PixelImageDisplay->setSizePolicy(sizePolicy3);
        PixelImageDisplay->setAutoFillBackground(true);
        PixelImageDisplay->setText(QString::fromUtf8(""));
        PixelImageDisplay->setScaledContents(true);

        verticalLayout_3->addWidget(PixelImageDisplay);

        ImageSize = new QSplitter(centralwidget);
        ImageSize->setObjectName("ImageSize");
        QSizePolicy sizePolicy4(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
        sizePolicy4.setHorizontalStretch(2);
        sizePolicy4.setVerticalStretch(2);
        sizePolicy4.setHeightForWidth(ImageSize->sizePolicy().hasHeightForWidth());
        ImageSize->setSizePolicy(sizePolicy4);
        ImageSize->setOrientation(Qt::Orientation::Horizontal);
        OutputSizeSpinner = new QSpinBox(ImageSize);
        OutputSizeSpinner->setObjectName("OutputSizeSpinner");
        OutputSizeSpinner->setMaximum(512);
        OutputSizeSpinner->setStepType(QAbstractSpinBox::StepType::DefaultStepType);
        ImageSize->addWidget(OutputSizeSpinner);
        ImgSizeLabel = new QLabel(ImageSize);
        ImgSizeLabel->setObjectName("ImgSizeLabel");
        ImgSizeLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        ImageSize->addWidget(ImgSizeLabel);

        verticalLayout_3->addWidget(ImageSize);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        PaletteSize = new QSplitter(centralwidget);
        PaletteSize->setObjectName("PaletteSize");
        QSizePolicy sizePolicy5(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy5.setHorizontalStretch(2);
        sizePolicy5.setVerticalStretch(2);
        sizePolicy5.setHeightForWidth(PaletteSize->sizePolicy().hasHeightForWidth());
        PaletteSize->setSizePolicy(sizePolicy5);
        PaletteSize->setOrientation(Qt::Orientation::Horizontal);
        PaletteSizeSpinner = new QSpinBox(PaletteSize);
        PaletteSizeSpinner->setObjectName("PaletteSizeSpinner");
        PaletteSize->addWidget(PaletteSizeSpinner);
        label = new QLabel(PaletteSize);
        label->setObjectName("label");
        label->setAlignment(Qt::AlignmentFlag::AlignCenter);
        PaletteSize->addWidget(label);

        verticalLayout->addWidget(PaletteSize);


        verticalLayout_3->addLayout(verticalLayout);


        horizontalLayout->addLayout(verticalLayout_3);


        verticalLayout_2->addLayout(horizontalLayout);

        MainWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        ImageUpload->setText(QCoreApplication::translate("MainWindow", "Upload Image", nullptr));
        ImageDownload->setText(QCoreApplication::translate("MainWindow", "Download Image", nullptr));
        StartPixelizet->setText(QCoreApplication::translate("MainWindow", "Pixelizet!", nullptr));
#if QT_CONFIG(whatsthis)
        OutputSizeSpinner->setWhatsThis(QString());
#endif // QT_CONFIG(whatsthis)
        OutputSizeSpinner->setSuffix(QCoreApplication::translate("MainWindow", " px", nullptr));
        ImgSizeLabel->setText(QCoreApplication::translate("MainWindow", "Output Image Size", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Output Palette Size", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
