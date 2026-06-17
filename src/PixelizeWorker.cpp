#include "PixelizeWorker.h"

void PixelizeWorker::process() {
    //std::cout << " STARTED" << std::endl;
    Pixelizer::PixelizationOutput result = Pixelizer::PixelizeImage(input);
    emit finished(result);
}