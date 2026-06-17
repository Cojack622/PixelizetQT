#include "Pixelizer.hpp"

#include <thread>

namespace Pixelizer {
    struct PThreadSettings {
        int numThreads;
        bool useMaxThreads;
    } _pThreadSettings;


    void InitializeParallelization() {}

    void AssociateRegionToSuperPixel(cv::Mat& sourceMat, std::vector<SuperPixel>& sPixels, std::vector<cv::Point2i>& regions, std::vector<std::mutex>& sourceMtx, cv::Point2i startPoint, cv::Point2i size) {
        int rEnd = startPoint.x + size.x;
        int cEnd = startPoint.y + size.y;

        if(rEnd < sourceMat.rows && sourceMat.rows < startPoint.x + 2 * size.x) {
            rEnd = sourceMat.rows;
        }
        if(cEnd < sourceMat.cols && sourceMat.cols < startPoint.y + 2 * size.y) {
            cEnd = sourceMat.cols;
        }

        for (int r = startPoint.x; r < rEnd; r++) {
            for (int c = startPoint.y; c < cEnd; c++) {
                Pixel currentPixel(sourceMat.at<cv::Vec3f>(r, c), cv::Point2f(c, r));

                int cOut = (int)((c / (float)sourceMat.cols) * _inputSettings.wOut);
                int rOut = (int)((r / (float)sourceMat.rows) * _inputSettings.hOut);

                int index = cOut + rOut * _inputSettings.wOut;
                int minIndex = index;
                double minDistance = sPixels[index].differenceCost(currentPixel, _pixelizationContext.differenceCoeff);

                
                for(int i = 1; i < regions.size(); i++){
                    if(inBounds(cOut + regions[i].x, rOut + regions[i].y, _inputSettings.wOut, _inputSettings.hOut)) {
                        index = cOut + regions[i].x + (rOut + regions[i].y) * _inputSettings.wOut;
                        double dist = sPixels[index].differenceCost(currentPixel, _pixelizationContext.differenceCoeff);
                        if (dist < minDistance) {
                            minDistance = dist;
                            minIndex = index;
                        }
                    }
                }

                sourceMtx[minIndex].lock();
                sPixels[minIndex].addPixel(std::move(currentPixel));
                sourceMtx[minIndex].unlock();
            }
        }
    }

    //Kinda dumb but just incase i need to set anything else up
    void InitializePThreads(){
        _pThreadSettings.numThreads = std::thread::hardware_concurrency();
    }

    void AssociatePixelsToSuperPixel() {
        InitializePThreads();

        int N = _inputSettings.hOut * _inputSettings.wOut;
        int inputC = _pixelizationContext.sourceMat.cols;
        int inputR = _pixelizationContext.sourceMat.rows;
        
        for (int i = 0; i < N; i++) {
            _pixelizationContext.superPixels[i].clearPixels();
        }

        const cv::Mat& sourceMat = _pixelizationContext.sourceMat;
        int rSize = inputR / _pThreadSettings.numThreads;
        int cSize = inputC / _pThreadSettings.numThreads;

        std::vector<std::thread> threads(_pThreadSettings.numThreads);
        std::vector<std::mutex> sourceMtx(_pixelizationContext.superPixels.size());

        for (int r = 0; r < _pThreadSettings.numThreads; r++)  {
            for (int c = 0; c < _pThreadSettings.numThreads; c++) {
                threads[c] = std::thread(AssociateRegionToSuperPixel, 
                    std::ref(_pixelizationContext.sourceMat), 
                    std::ref(_pixelizationContext.superPixels), 
                    std::ref(_pixelizationContext.regions), 
                    std::ref(sourceMtx), 
                    cv::Point2i(r * rSize, c * cSize),
                    cv::Point2i(rSize, cSize)
                );
            }

            for(int c = 0; c < _pThreadSettings.numThreads; c++){
                threads[c].join();
            }
        }

        for(int i =0; i < N; i++) {
            _pixelizationContext.superPixels[i].setAverages();
        }
    }
}

