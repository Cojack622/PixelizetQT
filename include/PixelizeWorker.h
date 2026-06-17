#pragma once

#include <QObject>
#include <Pixelizer.hpp>

class PixelizeWorker : public QObject
{
    Q_OBJECT
public:
    Pixelizer::PixelizationInputSettings input;

public slots:
    void process();

signals:
    void finished(const Pixelizer::PixelizationOutput &result);
};