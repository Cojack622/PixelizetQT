#pragma once

#include "Pixelizer_export.h"

// #ifdef PIXELIZER_USE_OPENCL
// #define CV_DISABLE_OPENCL   
// #define CL_HPP_TARGET_OPENCL_VERSION 300
// #include <CL/opencl.hpp>
// #endif

#include <opencv2/opencv.hpp>

#ifdef CL_RGBA
    #pragma message("CL_RGBA defined by opencv")
#endif
#include <vector>

namespace Pixelizer {


// Helper Functions
inline double ColorDifference(cv::Vec3f color1, cv::Vec3f color2) {
    cv::Vec3f diff = color2 - color1;
    return cv::norm(diff);
}

inline double PositionDifference(cv::Point2f point1, cv::Point2f point2){
    cv::Point2f diff = point2 - point1;
    return cv::norm(diff);
}


inline bool inBounds(cv::Point2i point, int w, int h) {
    return (point.x >= 0 && point.x < w && point.y >= 0 && point.y < h);
}

inline bool inBounds(int x, int y, int w, int h) {
    return (x >= 0 && x < w && y >= 0 && y < h);
}


const size_t NUM_COLORS = 3;

PIX_EXPORT typedef void (*OnTemperatureDecrease) (uchar** imgData, uint width, uint height, const char *imgFormat);

struct PIX_EXPORT PixelizationSettings {

    // Optimization Constants
    size_t PixelSearchDiameter = 15;
    size_t MaxLargeDimensionInput = 4000;

    // Annealing Constants
    float Alpha = 0.7f;
    float Beta = 1.1;
    float PaletteSplitEpsilon = 1.0f;
    float ClusterSplitEpsilon = 0.8f;
    float ClusterPerturbEpsilon = 0.75f;
    size_t ColorDistanceWeight = 45;
    double FinalTemp = 1.0;

    float InitialTempScaleFactor = 1.0f;
    float InitialTempIncreaseFactor = 1.1f;

    // Bilateral Filter Constants
    size_t BilateralFilterDiameter = 3;
    size_t BilateralFilterAlpha = 15;

};

inline PixelizationSettings _hyperparameters;


struct PIX_EXPORT PixelizationInputSettings {
    int scale;
    int wOut, hOut;
    int paletteSize;

    std::string inputFilePath;
    std::string settingsFilePath;
};
inline PixelizationInputSettings _inputSettings;

struct PIX_EXPORT PixelizationOutput {
    cv::Mat output;
    int pixelizationSuccess;
};


//Structs
struct Pixel {
    cv::Vec3f color;
    cv::Point2f position;

    Pixel(const cv::Vec3f color, const cv::Point2f position) : color(color), position(position){}
};

struct Cluster {
    cv::Vec3f color;
    size_t sub1Index, sub2Index;
};

class SuperPixel {
    private:
        cv::Vec3d _labAvg;
        cv::Point2d _summedPosition;

    public:
        cv::Vec3f averageColor;
        cv::Vec3f paletteColor;
        std::vector<Pixel> assosciatedPixels;
        std::vector<double> clusterProbabilities;

        cv::Point2f avgPixelPosition;

        void addToAvgColor(const cv::Vec3f& color){
            _labAvg += color;
        }

        void setAverages(){
            int setSize = assosciatedPixels.size();
            averageColor = _labAvg / (float)setSize;
            avgPixelPosition = _summedPosition / setSize;
        }

        void addPixel(Pixel&& pix){
            _summedPosition += (cv::Point2d)pix.position;
            _labAvg += pix.color;

            assosciatedPixels.push_back(std::move(pix));
        }

        void clearPixels(){
            assosciatedPixels.clear();

            _labAvg = cv::Vec3d(0, 0, 0);
            _summedPosition = cv::Point2d(0, 0);
        }

        void normalizeProbabilities(double normalizationFactor){
           for(double& prob : clusterProbabilities){
                prob /= normalizationFactor;
           }
        }

        double differenceCost(Pixel pix, double diffCoeff) {
            return ColorDifference(pix.color, paletteColor) + diffCoeff * PositionDifference(pix.position, avgPixelPosition);
        }
};

class PaletteColor {
    private: 


        double weight;
        cv::Vec3f color;

    public:

        std::vector<Pixel> associatedPixels;


        PaletteColor() = default;

        PaletteColor(const PaletteColor& copy){
            this->color = copy.color;

            associatedPixels = copy.associatedPixels;
            weight = copy.weight;
        }

        void setWeight(double p){
            weight = p;
        }

        void calculateCenter(){
            cv::Vec3d avg = cv::Vec3d::zeros();

            for(const Pixel& pix : associatedPixels){
                avg += pix.color;
            }

            avg = avg / (double)associatedPixels.size();
            color = avg;
        }

        void perturb(cv::Vec3d direction, double mod){
            color += direction * _hyperparameters.ClusterPerturbEpsilon * mod;
        }

        void updateWeight(std::vector<SuperPixel>& sPixels, size_t clusterIdx, double probSuperPixel){
            double tempWeight = 0.0;
            for(const SuperPixel& sPixel : sPixels){
                tempWeight += sPixel.clusterProbabilities[clusterIdx] * probSuperPixel;
            }
            weight = tempWeight;
        }

        cv::PCA createPCA() {
            
            int size = associatedPixels.size();
            if(size <= 0){
                std::cout << "Error: No associated pixels to create PCA." << std::endl;
            }
            cv::Mat data(size, NUM_COLORS, CV_32F);
            for(size_t i = 0; i < associatedPixels.size(); i++){
                data.at<float>(i, 0) = associatedPixels[i].color[0];
                data.at<float>(i, 1) = associatedPixels[i].color[1];
                data.at<float>(i, 2) = associatedPixels[i].color[2];
            }
            cv::Mat mean(1, NUM_COLORS, CV_32F);
            mean.at<float>(0, 0) = color[0];
            mean.at<float>(0, 1) = color[1];
            mean.at<float>(0, 2) = color[2];

            cv::PCA pca(data, mean, cv::PCA::DATA_AS_ROW, 3);
            return pca;
        }

        cv::Vec3f getColor() const {
            return color;
        }

        void setColor(cv::Vec3f newColor) {
            this->color = newColor;
        }

        double getWeight() const {
            return weight;
        }
    public:
        static cv::Vec3f average(PaletteColor& c1, PaletteColor& c2){
            return (c1.getColor() + c2.getColor()) / 2.0f;
        }
};

struct PixelizationContext {

    bool Initialized;

    PixelizationInputSettings inputSettings;

    double temp;
    double differenceCoeff;

    // Function Pointer that will return current superCluster output for display 
    OnTemperatureDecrease TempDecreaseEvent;

    std::vector<Cluster> clusters;
    std::vector<PaletteColor> palette;
    std::vector<SuperPixel> superPixels;
    std::vector<cv::Point2i> regions;

    size_t N, M;
    cv::Mat sourceMat;
    cv::Mat bilateralMat;
    cv::Mat sourceMatAlpha;
};
inline PixelizationContext _pixelizationContext;




std::vector<cv::Point2i> GenerateSearchVector(size_t H, size_t W);


// Image Smoothing Functions
cv::Mat BilateralFilterMat();
std::vector<cv::Point2f> LapachianSmooth(float movePercentage);

// Setup Functions
PaletteColor InitializeSuperPixels();
double InitializePixelizationAlgorithm();
cv::Mat CreateInputMat(const std::string& filePath, int outputScale);
std::vector<cv::Point2i> GenerateSearchVector();

// Math Functions
double SuperPixelMapsClusterProb(SuperPixel& sPixel, PaletteColor& color, cv::Vec3f bilateralColor);
void CalculateSuperToClusterProbabilities();
//Change later to use importance map
double PixelMapsToSuperPixelProb();

// Annealing/SP functions
void RefinePalette(std::vector<cv::Vec3f>& oldPalette);
double PaletteChange(std::vector<cv::Vec3f>& oldPalette);
void MergeSubClusters();
void PaletteExpand();
void PerturbSubClusters(Cluster& cluster);
 
// API Functions
PIX_EXPORT PixelizationOutput PixelizeImage(PixelizationInputSettings inputSettings, PixelizationSettings hyperparameters = _hyperparameters);
PIX_EXPORT void NewSuperPixelOutputSubrscribe(OnTemperatureDecrease decreaseEvent);

// Output Functions
cv::Mat SuperPixelOutput(uint scaleFactor, bool useBGR);

// System-Specific Functions
void InitializeParallelization();
void AssociatePixelsToSuperPixel();
}