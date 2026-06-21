#include "Pixelizer.hpp"
#include "PixelizerLog.hpp"
#include <iostream>


#include <random>
//TODO 
//Maintain an SuperPixel averages image so we dont have to calcuate it in loop
namespace Pixelizer {


    inline void DrawSuperPixels() {
        cv::Mat output = cv::Mat(_pixelizationContext.sourceMat.rows, _pixelizationContext.sourceMat.rows, CV_8UC3);

        std::random_device rd;
    
    // 2. Initialize the standard mersenne_twister_engine with the seed
        std::mt19937 gen(rd());
    
    // 3. Define the range [inclusive, inclusive]
        std::uniform_int_distribution<int> distrib(0, 255);

    // 4. Generate the random number

        for(const SuperPixel& sPixel : _pixelizationContext.superPixels){
            cv::Vec3b color = cv::Vec3b(distrib(gen), distrib(gen), distrib(gen));

            if(sPixel.assosciatedPixels.empty()) {
                continue;
            }

            for(const Pixel& pix : sPixel.assosciatedPixels){
                output.at<cv::Vec3b>((int)pix.position.x, (int)pix.position.y) = color;
            }

        }


        cv::Mat scaledOutput(_pixelizationContext.sourceMat.rows / 2, _pixelizationContext.sourceMat.cols / 2, CV_8UC3);
        cv::resize(output, scaledOutput, scaledOutput.size(), 0, 0, cv::INTER_NEAREST);

        cv::imshow("SuperPixels", scaledOutput);
        cv::waitKey(0);
    }


    #define DATA_TYPE CV_32FC3
    inline cv::Mat BilateralFilterMat(){
        int H = _inputSettings.hOut, W = _inputSettings.wOut;
        cv::Mat averages(H, W, DATA_TYPE);
        cv::Mat bilateralFilter(H, W, DATA_TYPE);

        for(int r = 0; r < H; r++){
            for(int c = 0; c < W; c++){
                averages.at<cv::Vec3f>(r, c) = _pixelizationContext.superPixels[c + r * W].averageColor;
            }
        }

        cv::bilateralFilter(averages, bilateralFilter, 
            _hyperparameters.BilateralFilterDiameter, 
            _hyperparameters.BilateralFilterAlpha, 
            _hyperparameters.BilateralFilterAlpha);

        return bilateralFilter;
    }


    #define NUM_NEIGHBORS 4

    inline std::vector<cv::Point2f> LapachianSmooth(float movePercentage) {
        int H = _inputSettings.hOut, W = _inputSettings.wOut;


        //Map to more easily index N, W, S, E neighbors
        cv::Point2i regions[] = {cv::Point2i(0,1), cv::Point2i(1,0), cv::Point2i(0,-1), cv::Point2i(-1, 0)};
        std::vector<cv::Point2f> output;

        auto& superPixels = _pixelizationContext.superPixels;

        for (int r = 0; r < H; r++){
            for (int c = 0; c < W; c++){
                SuperPixel& pixel = superPixels[c + r * W];

                cv::Point2f avg(0, 0);
                int dividend = 0;

                for (int i = 0; i < NUM_NEIGHBORS; i++){
                    cv::Point2i check = cv::Point2i(c, r) + regions[i];
                    if (inBounds(check, W, H)){
                        avg += superPixels[check.x + check.y * W].avgPixelPosition;

                        dividend++;
                    }
                }

                avg = avg / (float)dividend;

                cv::Point2f newPos = pixel.avgPixelPosition + movePercentage * (avg - pixel.avgPixelPosition);
                output.push_back(newPos);
            }
        }

        return output;
    }

    inline cv::Mat CreateInputMat(const std::string& filePath, int outputScale){
        cv::Mat inputImage = cv::imread(filePath, cv::IMREAD_COLOR);
        int wScale = inputImage.cols;
        int hScale = inputImage.rows;
        
        int maxDim = _hyperparameters.MaxLargeDimensionInput;


        float reductionFactor;
        if (inputImage.cols > inputImage.rows) {
            reductionFactor = (inputImage.rows / (float)inputImage.cols);
            if(wScale > maxDim) {
                wScale = maxDim;
                hScale = hScale = maxDim * reductionFactor;
            }

            _inputSettings.wOut = outputScale;
            _inputSettings.hOut = outputScale * reductionFactor;
        }else if (inputImage.rows > inputImage.cols) {
            reductionFactor = (inputImage.cols / (float)inputImage.rows);
            if(hScale > maxDim) {
                hScale = maxDim;
                wScale = maxDim * reductionFactor;
            }

            _inputSettings.hOut = outputScale;
            _inputSettings.wOut = outputScale * reductionFactor;
        }else {
            if(hScale > maxDim) {
                hScale = maxDim;
                wScale = maxDim;
            }
            _inputSettings.hOut = outputScale;
            _inputSettings.wOut = outputScale;
        }


        cv::Mat scaledImage;
        cv::resize(inputImage, scaledImage, cv::Size(wScale, hScale));

        inputImage.release();

        cv::Mat floatImage(scaledImage.rows, scaledImage.cols, CV_32FC3);
        scaledImage.convertTo(floatImage, CV_32F, 1.0 / 255.0);

        scaledImage.release();

        cv::Mat sourceMat(floatImage.rows, floatImage.cols, CV_32FC3);
        cv::cvtColor(floatImage, sourceMat, cv::COLOR_BGR2Lab);
        
        floatImage.release();

        return sourceMat;
    }


    inline PaletteColor InitializeSuperPixels() {

        auto& sourceMat = _pixelizationContext.sourceMat;
        const int H = _inputSettings.hOut, W = _inputSettings.wOut;

        PaletteColor firstColor;
        int fullSuperPixelWidth = sourceMat.cols / W;
        int fullSuperPixelHeight = sourceMat.rows / H;

        int rCount = 0, cCount = 0;

        //No real logical reason to do a one pass loop rather than 2 
        //Not sure why i wrote it this way, but id rather not risk some unknown bug due to rewriting as
        //A 2 pass loop
        int sPixelCount = W * H;
        for (int i = 0; i < sPixelCount; i++){
            
            SuperPixel sPixel;
            sPixel.avgPixelPosition.x = (fullSuperPixelWidth * cCount) + (fullSuperPixelWidth / 2);
            sPixel.avgPixelPosition.y = (fullSuperPixelHeight * rCount) + (fullSuperPixelHeight / 2);

            cCount++;

            if(cCount >= W){
                cCount = 0;
                rCount++;
            }

            _pixelizationContext.superPixels.push_back(sPixel);
        }


        //Extremely Parallelizeable
        for (int r = 0; r < sourceMat.rows; r++) {
            for(int c = 0; c < sourceMat.cols; c++) {

                int cOut = (int)((c / (float)sourceMat.cols) * W);
                int rOut = (int)((r / (float)sourceMat.rows) * H);

                int currentIndex = cOut + rOut * W;


                //cv::Point2i testPoint(c, r);
                cv::Vec3f pixelColor = sourceMat.at<cv::Vec3f>(r, c);
                cv::Point2i pixelPosition(c, r);

                Pixel pixel(pixelColor, pixelPosition);
                (_pixelizationContext.superPixels)[currentIndex].addPixel(std::move(pixel));
                //(_pixelizationContext.superPixels)[currentIndex].addToAvgColor(pixelColor);

                firstColor.associatedPixels.push_back(pixel);
            }
        }

        // for(const SuperPixel& sPixel : _pixelizationContext.superPixels){

        //     if(sPixel.assosciatedPixels.empty()) {
        //         continue;
        //     }
        // }

        firstColor.calculateCenter();

        for (int i = 0; i < W * H; i++){
            (_pixelizationContext.superPixels)[i].setAverages();
            (_pixelizationContext.superPixels)[i].paletteColor = firstColor.getColor();
        }

        return firstColor;

    }

    inline double InitializePixelizationAlgorithm(){
        PaletteColor criticalCluster = InitializeSuperPixels();

        criticalCluster.setWeight(1.0f);

        cv::PCA wholeImagePCA = criticalCluster.createPCA();
        PaletteColor criticalClusterCopy(criticalCluster);
        cv::Vec3f principleAxis = cv::Vec3f(
            wholeImagePCA.eigenvectors.at<float>(0,0),
            wholeImagePCA.eigenvectors.at<float>(0,1),
            wholeImagePCA.eigenvectors.at<float>(0,2)
        );

        criticalCluster.perturb(principleAxis, 1);
        criticalClusterCopy.perturb(principleAxis, -1);

        _pixelizationContext.palette.reserve(_inputSettings.paletteSize*2);


        _pixelizationContext.palette.push_back(criticalCluster);
        _pixelizationContext.palette.push_back(criticalClusterCopy);


        Cluster c1;
        c1.color = PaletteColor::average(criticalCluster, criticalClusterCopy);
        c1.sub1Index = 0;
        c1.sub2Index = 1;
        _pixelizationContext.clusters.push_back(c1);

        double tc = _hyperparameters.InitialTempIncreaseFactor * wholeImagePCA.eigenvalues.at<float>(0);
        return 2.0 * tc /*/ _hyperparameters.InitialTempScaleFactor*/;
    }

    inline double SuperPixelMapsClusterProbability(SuperPixel& sPixel, PaletteColor& color, cv::Vec3f bilateralColor){
        double p = color.getWeight() * std::exp(-1 * ColorDifference(bilateralColor, color.getColor()) / _pixelizationContext.temp);
        sPixel.clusterProbabilities.push_back(p);
        return p;
    }

    inline std::pair<int, double> GetMaxClusterProbability(SuperPixel& sPixel, cv::Vec3f bilateralColor){
        // Following can be done by including std::algorithms for a more idiomatic approach
        // However this is more readable even if its ugly
        int pSize = _pixelizationContext.palette.size();

        if (pSize == 0){
            throw std::length_error("Process failed, palette has not been initialized.");
        }    

        double max = SuperPixelMapsClusterProbability(sPixel, _pixelizationContext.palette[0], bilateralColor);
        double sumNorm = max;

        // Find maximumum probability
        int maxIdx = 0;
        for(int i = 1; i < pSize; i++){
            double temp = SuperPixelMapsClusterProbability(sPixel, _pixelizationContext.palette[i], bilateralColor);
            sumNorm += temp;
            if(temp > max){
                max = temp;
                maxIdx = i;
            }
        }

        return std::make_pair(maxIdx, sumNorm);
    }

    inline void CalculateProbabilities(){
        
        const int H = _inputSettings.hOut, W = _inputSettings.wOut;
        auto& palette = _pixelizationContext.palette;

        for (int r = 0; r < H; r++){
            for(int c = 0; c < W; c++) {
                SuperPixel& sPixel = _pixelizationContext.superPixels[c + r * W];
                sPixel.clusterProbabilities.clear();

                cv::Vec3f bilateralColor = _pixelizationContext.bilateralMat.at<cv::Vec3f>(r, c);

                std::pair<int, double> maxProb = GetMaxClusterProbability(sPixel, bilateralColor);

                sPixel.normalizeProbabilities(maxProb.second);
                sPixel.paletteColor = palette[maxProb.first].getColor();
            }
        }
        
        double numSuperPixels = (double)_pixelizationContext.superPixels.size();
        for (int i = 0; i < _pixelizationContext.palette.size(); i++){
            palette[i].updateWeight(_pixelizationContext.superPixels, i, 1.0 / numSuperPixels);
        }


    }

    inline std::vector<cv::Point2i> GenerateSearchVector() {
        int searchBoxDiameter = _hyperparameters.PixelSearchDiameter;
        std::vector<cv::Point2i> regions;
        for (int i = -(searchBoxDiameter / 2); i < (searchBoxDiameter / 2) + 1; i++) {
            for (int j = -(searchBoxDiameter / 2); j < (searchBoxDiameter / 2) + 1; j++) {

                if (i != 0 || j != 0) {
                    regions.push_back(cv::Point2i(i, j));
                }
            }
        }

        return regions;
    }


    inline double PixelMapsToSuperPixelProb(){
        const int H = _inputSettings.hOut, W = _inputSettings.wOut;
        return 1 / (double)(W * H);
    }

    inline void RefinePalette(std::vector<cv::Vec3f>& oldPalette){

        const int H = _inputSettings.hOut, W = _inputSettings.wOut;
        auto& palette = _pixelizationContext.palette;


        oldPalette.clear();
        double probSuperPixel = PixelMapsToSuperPixelProb();

        //PARALLELIZEABLE
        //Probably a 0.5-1 second bottleneck
        for (int cluster = 0; cluster < palette.size(); cluster++){
            cv::Vec3f refinedColor = cv::Vec3f::zeros();
            for (int r = 0; r < H; r++){
                for(int c = 0; c < W; c++) {
                    cv::Vec3f M = _pixelizationContext.bilateralMat.at<cv::Vec3f>(r, c);
                    double probSuperPixelMapsCluster = _pixelizationContext.superPixels[c + r * W].clusterProbabilities[cluster];
                    refinedColor += M * probSuperPixelMapsCluster * probSuperPixel;
                }
            }        

            oldPalette.push_back(palette[cluster].getColor());
            palette[cluster].setColor(refinedColor / palette[cluster].getWeight());
        }

    }

    inline double PaletteChange(std::vector<cv::Vec3f>& oldPalette){
        double summedDiff = 0.0;
        for (int i = 0; i < _pixelizationContext.palette.size(); i++){
            summedDiff += ColorDifference(oldPalette[i], _pixelizationContext.palette[i].getColor());
        }

        return summedDiff;
    }

    inline void MergeSubClusters() {
        auto& palette = _pixelizationContext.palette;
        auto& clusters = _pixelizationContext.clusters;

        std::vector<PaletteColor> finalPalette;
        for (int c = 0; c < clusters.size(); c++){
            Cluster& cluster = clusters[c];
            PaletteColor finalColor = palette[cluster.sub1Index];

            finalColor.setWeight(finalColor.getWeight() + palette[cluster.sub2Index].getWeight());
            
            finalColor.setColor((finalColor.getColor() + palette[cluster.sub2Index].getColor()) / 2.0);
            //finalColor += palette[cluster.sub2Index].getColor();
            //finalColor /= 2.0;
            finalPalette.push_back(finalColor);
        }

        _pixelizationContext.palette = std::move(finalPalette);
    }


    inline void PerturbSubClusters(Cluster& cluster) {
        auto& palette = _pixelizationContext.palette;

        PaletteColor& ck1 = palette[cluster.sub1Index];
        PaletteColor& ck2 = palette[cluster.sub2Index];

        cv::PCA pcaAnalysis = ck1.createPCA();
        cv::Vec3f perturbDir = cv::Vec3f(
            pcaAnalysis.eigenvectors.at<float>(0,0),
            pcaAnalysis.eigenvectors.at<float>(0,1), 
            pcaAnalysis.eigenvectors.at<float>(0,2)
        );
                
        palette[cluster.sub1Index].perturb(perturbDir, 1);
        palette[cluster.sub2Index].perturb(perturbDir, -1);
    }
    
    int itCounter = 0;

    //int PalExpCallsSinceLast = 0;
    inline void PaletteExpand() {

        int i = itCounter;
        int size = _pixelizationContext.clusters.size();

        auto& palette = _pixelizationContext.palette;
        auto& clusters = _pixelizationContext.clusters;

        //PalExpCallsSinceLast++;
        //itCounter++;
        for (int c = 0; c < size; c++) {
            if(clusters.size() == _inputSettings.paletteSize){
                break;
            }

            Cluster& cluster = clusters[c];
            PaletteColor& ck1 = palette[cluster.sub1Index];
            PaletteColor& ck2 = palette[cluster.sub2Index];

            double split = ColorDifference(ck1.getColor(), ck2.getColor());


            // P :: [x, x, c1, x, x, c2, c11, c22]
            // C :: [(2, 5)]

            // oC :: (2,6)
            // nC :: (5,7)
            // if(PalExpCallsSinceLast > 30){
            //     std::cout<<"Palette expansion called more than 30 times since last merge"<<std::endl;
            // }
            if(split > _hyperparameters.ClusterSplitEpsilon){
                // New cluster, representing the 2nd index of the split cluster
                Cluster newCluster;
                newCluster.sub1Index = cluster.sub2Index;

                //Purposefully copy 
                std::vector<Pixel> fullCluster = ck1.associatedPixels;
                ck1.associatedPixels.clear();
                ck2.associatedPixels.clear();

                int clusterSize = fullCluster.size();
                for (int i = 0; i < clusterSize; i++){
                    Pixel& pixel = fullCluster[i];

                    double ck1Diff = ColorDifference(pixel.color, ck1.getColor());
                    double ck2Diff = ColorDifference(pixel.color, ck2.getColor());

                    if(ck1Diff < ck2Diff){
                        ck1.associatedPixels.push_back(pixel);
                    }
                    else{
                        ck2.associatedPixels.push_back(pixel);
                    }
                }


                if(ck1.associatedPixels.empty() || ck2.associatedPixels.empty()){
                    std::cout << "Error: One or both clusters are empty." << std::endl;
                    int size1 = ck1.associatedPixels.size();
                    int size2 = ck2.associatedPixels.size();
                    std::cout << "Sizes: " << size1 << ", " << size2 << std::endl;
                    return;
                }


                //ck1.setWeight(ck1.getWeight() / 2.0);
                PaletteColor ck1Copy(ck1);
                palette.push_back(ck1Copy);

                
                cluster.sub2Index = palette.size() - 1;
                cluster.color = ck1.getColor();

                //Must redefine ck2 since palette may have reallocated when pushing back ck1Copy
                PaletteColor& ck2 = palette[newCluster.sub1Index];
                //ck2.setWeight(ck2.getWeight() / 2.0);
                PaletteColor ck2Copy(ck2);
                palette.push_back(ck2Copy);

                newCluster.sub2Index = palette.size() - 1;
                newCluster.color = ck2.getColor();


                //NEED either a call to calculate the center of the new clusters or a way to update their weights
                ck1.calculateCenter();
                ck2.calculateCenter();


                //Perturb the 2 new clusters
                PerturbSubClusters(cluster);
                PerturbSubClusters(newCluster);

                clusters.push_back(newCluster);
                
                if(clusters.size() == _inputSettings.paletteSize){
                    MergeSubClusters();
                }
                //PalExpCallsSinceLast = 0;
            }
            else{
                PerturbSubClusters(cluster);
            }

        }

    }

    inline cv::Mat SuperPixelOutput(uint scaleFactor = 1, bool useBGR = true){
        //float beta = 1.1f;
        cv::Mat labOutput(_inputSettings.hOut, _inputSettings.wOut, CV_8UC3);

        for (int r = 0; r < _inputSettings.hOut; r++){
            for (int c = 0; c < _inputSettings.wOut; c++){
                SuperPixel& sPixel = _pixelizationContext.superPixels[c+r*_inputSettings.wOut];
                labOutput.at<cv::Vec3b>(r, c) = cv::Vec3b(
                    (unsigned char)(sPixel.paletteColor[0] * (255.0/100.0)), 
                    (unsigned char)(sPixel.paletteColor[1] * _hyperparameters.Beta - 128),
                    (unsigned char)(sPixel.paletteColor[2] * _hyperparameters.Beta - 128)
                );
            }
        }

        cv::Mat scaledOutput(_inputSettings.hOut * scaleFactor, _inputSettings.wOut * scaleFactor, CV_8UC3);
        cv::resize(labOutput, scaledOutput, scaledOutput.size(), 0, 0, cv::INTER_NEAREST);

        cv::Mat rgbOutput(_inputSettings.hOut, _inputSettings.wOut, CV_8UC3);
        cv::cvtColor(scaledOutput, rgbOutput, cv::COLOR_Lab2BGR);

        scaledOutput.release();
        labOutput.release();


        return rgbOutput;
    }

    bool VerifyPaletteWeightsUniform() {
        double sum = 0.0;
        for(const PaletteColor& color : _pixelizationContext.palette){
            sum += color.getWeight();
        }
        return (sum > 0.99 && sum < 1.01);
    }

    bool VerifyPaletteProbabilitiesUniform(){
        for(SuperPixel& sPixel : _pixelizationContext.superPixels){

            double u = 0.0;
            for (double prob : sPixel.clusterProbabilities) {
                u += prob;
            }

            if(u < 0.99 || u > 1.01){
                return false;
            }
        }
        return true;
    }
    
    PIX_EXPORT PixelizationOutput PixelizeImage(PixelizationInputSettings inputSettings, PixelizationSettings hyperparameters) {

        if(_pixelizationContext.Initialized){
            PixelizationOutput out;
            out.pixelizationSuccess = -1;
            return out;
        }

        //PixLogger logger("C:/Users/cojac/PixelQT/output/log.txt");


        _pixelizationContext.Initialized = true;
        
        _inputSettings = inputSettings;
        _hyperparameters = hyperparameters;

        _pixelizationContext.sourceMat = CreateInputMat(_inputSettings.inputFilePath, _inputSettings.scale);

        int N = _inputSettings.hOut * _inputSettings.wOut;
        int M = _pixelizationContext.sourceMat.rows * _pixelizationContext.sourceMat.cols;
        _pixelizationContext.differenceCoeff = _hyperparameters.ColorDistanceWeight * std::sqrt(N / (double)M);

        _pixelizationContext.temp = InitializePixelizationAlgorithm();
        _pixelizationContext.regions = GenerateSearchVector();

        _pixelizationContext.bilateralMat = cv::Mat(_inputSettings.hOut, _inputSettings.wOut, CV_32FC3);
        std::vector<cv::Point2f> newPoints;
        std::vector<cv::Vec3f> oldColors;


        InitializeParallelization();
        while (_pixelizationContext.temp > _hyperparameters.FinalTemp) {

            if(newPoints.size() != 0) {
                for (int i = 0; i < _pixelizationContext.superPixels.size(); i++){
                    _pixelizationContext.superPixels[i].avgPixelPosition = newPoints[i];
                }
            }

            AssociatePixelsToSuperPixel();
            newPoints = LapachianSmooth(0.4f);
            _pixelizationContext.bilateralMat = BilateralFilterMat();

            CalculateProbabilities();

            RefinePalette(oldColors);
            double paletteChange = PaletteChange(oldColors);

            if(paletteChange < _hyperparameters.PaletteSplitEpsilon){

                //DrawSuperPixels();
                
                _pixelizationContext.temp *= _hyperparameters.Alpha;


                std::ofstream logFile("C:/Users/cojac/PixelQT/output/log.txt", std::ios_base::app);
                if (!logFile.is_open()) {
                    throw std::runtime_error("Failed to open log file: C:/Users/cojac/PixelQT/output/log.txt");
                }
                std::string logMessage = "\nIter: " + std::to_string(itCounter) 
                + "\n Temp: " + std::to_string(_pixelizationContext.temp)
                + "\n Clusters: " + std::to_string(_pixelizationContext.clusters.size())
                + "\n Palette: " + std::to_string(_pixelizationContext.palette.size());

                logFile << logMessage << std::endl;

                logFile.flush();
                logFile.close();
                //logger.Log(logMessage);

                if(_pixelizationContext.clusters.size() < _inputSettings.paletteSize){
                    PaletteExpand();
                }

                // Function Callback
                if (_pixelizationContext.TempDecreaseEvent != nullptr) {
                    #define SCALE_FACTOR 10
                    cv::Mat tempOut = SuperPixelOutput(SCALE_FACTOR);
                    //cv::imwrite("C:/Users/cojac/PixelQT/output/outputLarge.png", tempOut);

                    //std::cout << "Wrote to file\n";
                    _pixelizationContext.TempDecreaseEvent(&tempOut.data, tempOut.cols, tempOut.rows, "RGB");

                    tempOut.release();
                }
            }
            itCounter++;
        }


        // if(_pixelizationContext.clusters.size() < _inputSettings.paletteSize){
        //     MergeSubClusters();
        //     CalculateProbabilities();
        //     #define SCALE_FACTOR 10
        //     cv::Mat tempOut = SuperPixelOutput(SCALE_FACTOR);

        //     _pixelizationContext.TempDecreaseEvent(&tempOut.data, tempOut.cols, tempOut.rows, "RGB");
        //     tempOut.release();
        // }
        size_t numClusters = _pixelizationContext.clusters.size();
        size_t numColors = _pixelizationContext.palette.size();

        int itCount = itCounter;

        _pixelizationContext.Initialized = false;

        DrawSuperPixels();



        _pixelizationContext.bilateralMat.release();
        _pixelizationContext.sourceMat.release();


        PixelizationOutput output;
        output.pixelizationSuccess = 1;
        output.output = SuperPixelOutput();
        cv::imwrite("C:/Users/cojac/PixelQT/output/final.png", output.output);
        return output;
    } 

    PIX_EXPORT void NewSuperPixelOutputSubrscribe(OnTemperatureDecrease decreaseEvent) {
        _pixelizationContext.TempDecreaseEvent = decreaseEvent;
    }
}