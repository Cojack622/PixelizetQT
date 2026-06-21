#include "Pixelizer.hpp"
#define CL_HPP_TARGET_OPENCL_VERSION 300
#include <CL/opencl.hpp>

#include <fstream>
#include <optional>


#include <unordered_set>

namespace Pixelizer { 

    // Not a fan of using "using namespace" but expansion of CL macros fails 
    // whenever using cl::MACRO, since it expands to cl::EXPANDED <- Expected unqualified id blah blah
    // Then, when NOT using cl::MACRO, it has no idea what the macro is supposed to be.
    using namespace cl;

    //These are somehwat dumb and stupid but oh well
    #define NUM_COLORS_PER_SUPERPIXEL 3
    std::vector<float> CreateSuperPixelColors() {
        std::vector<float> superPixelData;
        superPixelData.reserve(_pixelizationContext.superPixels.size() * NUM_COLORS_PER_SUPERPIXEL);
        for(SuperPixel& sp : _pixelizationContext.superPixels) {
            superPixelData.push_back(sp.paletteColor[0]);
            superPixelData.push_back(sp.paletteColor[1]);
            superPixelData.push_back(sp.paletteColor[2]);
        }

        return superPixelData;
    }

    #define NUM_POSITIONS_PER_SUPERPIXEL 2
    std::vector<float> CreateSuperPixelPositions() {
        std::vector<float> superPixelData;
        superPixelData.reserve(_pixelizationContext.superPixels.size() * NUM_POSITIONS_PER_SUPERPIXEL);
        for(SuperPixel& sp : _pixelizationContext.superPixels) {
            superPixelData.push_back(sp.avgPixelPosition.x);
            superPixelData.push_back(sp.avgPixelPosition.y);
        }

        return superPixelData;
    }

    void AssignPixels(std::vector<uint>& associations) {
        for(SuperPixel& sp : _pixelizationContext.superPixels) {
            sp.clearPixels();
        }


        //std::unordered_set<uint> uniqueAssociations(associations.begin(), associations.end());
        //size_t uniqueCount = uniqueAssociations.size();

        for(size_t i = 0; i < associations.size(); i++) {
            Pixel p( 
                *(_pixelizationContext.sourceMat.ptr<cv::Vec3f>(i / _pixelizationContext.sourceMat.cols, i % _pixelizationContext.sourceMat.cols)),
                cv::Point2i(i % _pixelizationContext.sourceMat.cols, i / _pixelizationContext.sourceMat.cols)
            );
            _pixelizationContext.superPixels[associations[i]].addPixel(std::move(p));
        }

        for(SuperPixel& sp : _pixelizationContext.superPixels) {
            sp.setAverages();
            //int siz = sp.assosciatedPixels.size();
            //std::cout << "SuperPixel " << &sp - _pixelizationContext.superPixels.data() << " has " << siz << " associated pixels." << std::endl;
        }
    }


    void DrawAssociationMap(std::vector<uint>& associations) {
        cv::Mat associationMap(_pixelizationContext.sourceMat.rows, _pixelizationContext.sourceMat.cols, CV_8UC3);
        
        for (int r = 0; r < associationMap.rows; r++){
            for (int c = 0; c < associationMap.cols; c++){
                int flatIdx = c + r * associationMap.cols;
                int size = associations.size();
                uint associationIdx = associations[flatIdx];

 

                SuperPixel& sPixel = _pixelizationContext.superPixels[associationIdx];
                associationMap.at<cv::Vec3b>(r, c) = cv::Vec3b(
                    (unsigned char)(sPixel.paletteColor[0] * (255.0/100.0)), 
                    (unsigned char)(sPixel.paletteColor[1]  - 128),
                    (unsigned char)(sPixel.paletteColor[2] - 128)
                );
            }
        }

        cv::Mat rgbOutput(_pixelizationContext.sourceMat.rows, _pixelizationContext.sourceMat.cols, CV_8UC3);
        cv::cvtColor(associationMap, rgbOutput, cv::COLOR_Lab2BGR);

        cv::Mat scaledOutput(_pixelizationContext.sourceMat.rows / 2, _pixelizationContext.sourceMat.cols / 2, CV_8UC3);
        cv::resize(rgbOutput, scaledOutput, scaledOutput.size(), 0, 0, cv::INTER_NEAREST);

        cv::imshow("Association Map", scaledOutput);
        cv::waitKey(0);
    }


    // using AssociateP2SPKernel = cl::KernelFunctor<
    //     cl::Buffer, // Image
    //     cl::Buffer, // Colors
    //     cl::Buffer, // Positions
    //     cl::Buffer, // Source
    //     cl_float2, // InputImage Size (N)
    //     cl_float2, // OutputImage Size (M)
    //     cl_int // Position Importance Coeff
    // >;

    static const std::string kernelSource = R"(
        void kernel AssociatePixelsToSuperPixel(global const float* sourceMat, global const float* superPixelColors, global const float* superPixelPositions, global uint* associations, float2 M, float2 N, float diffCoeff) {
            int cIn = get_global_id(0);
            int rIn = get_global_id(1);


            int flatIdx = cIn + rIn * (int)M.x;

            //Ensure non-encroachment
            if(rIn < M.y && cIn < M.x) {
                float3 dPixel = vload3(flatIdx, sourceMat);

                float minCost = INFINITY;
                uint minSuperPixelIdx = 0;
                for(int i = 0; i < N.x * N.y; i++) 
                {

                    float3 spColor = vload3(i, superPixelColors);
                    float2 spPos = vload2(i, superPixelPositions);

                    float3 colorDiff = spColor - dPixel;
                    float2 posDiff = spPos - convert_float2((int2)(cIn, rIn));

                    float3 color = colorDiff * colorDiff;
                    float2 pos = posDiff * posDiff;

                    float cost = native_sqrt(color.x + color.y + color.z) + diffCoeff * native_sqrt(pos.x + pos.y);
                    if(cost < minCost) {
                        minCost = cost;
                        minSuperPixelIdx = i;
                    }
                }
                
                associations[flatIdx] = minSuperPixelIdx;

            }

        }
    )";
    
    struct AssociateP2SPContext {
        std::vector<cl::Device> devices;
        cl::Platform default_platform;
        cl::Device default_device;

        cl::Context context;
        cl::CommandQueue queue;
        cl::Kernel kernel;

        cl::Program::Sources sources;
        cl::Program program;

        cl::Buffer source_d;
        cl::Buffer output_d;

        //Handles, will change
        cl::Buffer colors_d;
        cl::Buffer positions_d;
    };

    //Optional so that i can initialize it later
    //Could probably be done via classes/a massive ugly constructor???
    std::optional<AssociateP2SPContext> _algCtx;

    void InitializeParallelization() {

        std::vector<cl::Device> devices;

        cl::Platform default_platform = cl::Platform::getDefault();
        // Requires GPU (CPU must use PThreads to maximize workload)
        default_platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        if(devices.empty()) {
            std::cerr << "No GPU devices found!" << std::endl;
            exit(1);
        }
        cl::Device default_device = devices[0];
        //Debug logging
        std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << std::endl;

        // Initialize OpenCL context 
        cl::Context context({default_device});

        //Regular float buffer since image is in CIELAB format. 
        // Epxlicit size of float4 
        cl::Buffer source_d(context, CL_MEM_READ_ONLY, /*sizeof(_pixelizationContext.sourceMat.data)*/ _pixelizationContext.sourceMat.rows * _pixelizationContext.sourceMat.cols * _pixelizationContext.sourceMat.elemSize());
        // This could be an image buffer since its not in LAB, but idk
        cl::Buffer output_d(context, CL_MEM_WRITE_ONLY, sizeof(uint) * _pixelizationContext.sourceMat.rows * _pixelizationContext.sourceMat.cols);


        size_t colorsSize = _pixelizationContext.superPixels.size() * sizeof(float) * NUM_COLORS_PER_SUPERPIXEL;
        size_t positionsSize = _pixelizationContext.superPixels.size() * sizeof(float) * NUM_POSITIONS_PER_SUPERPIXEL;
        cl::Buffer colors_d(context, CL_MEM_READ_ONLY, colorsSize);
        cl::Buffer positions_d(context, CL_MEM_READ_ONLY, positionsSize);



        cl::CommandQueue queue(context, default_device);
        queue.enqueueWriteBuffer(source_d, CL_TRUE, 0, _pixelizationContext.sourceMat.total() * _pixelizationContext.sourceMat.elemSize(), _pixelizationContext.sourceMat.data);

        // Naive cast SHOULD work 
        cl::Program::Sources sources;
        sources.push_back({kernelSource.c_str(), kernelSource.length()});
        cl::Program program(context, sources);

        if(program.build({default_device}) != CL_SUCCESS) {
            std::ofstream logFile("error.txt");
            std::string buildLog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device);
            logFile << "Error building program: " << buildLog << std::endl;
            logFile.close();
            exit(1);
        }

        cl::Kernel kernel(program, "AssociatePixelsToSuperPixel");
        kernel.setArg(0, source_d);
        kernel.setArg(1, colors_d);
        kernel.setArg(2, positions_d);
        kernel.setArg(3, output_d);
        kernel.setArg(4, cl_float2{(cl_float)_pixelizationContext.sourceMat.cols, (cl_float)_pixelizationContext.sourceMat.rows});
        kernel.setArg(5, cl_float2{(cl_float)_inputSettings.wOut, (cl_float)_inputSettings.hOut});
        kernel.setArg(6, (cl_float)_pixelizationContext.differenceCoeff);

        _algCtx = AssociateP2SPContext{
            devices,
            default_platform,
            default_device,
            context,
            queue,
            kernel,
            sources,
            program,
            source_d,
            output_d,
            colors_d,
            positions_d
        };

    }


    //Output image for which superpixel it belongs to 



    int AssociateCount = 0;
    // Associate pixels to superpixels using OpenCL 
    void AssociatePixelsToSuperPixel() { 
        // Initialize OpenCL  devices
        

        //Slightly wasteful to create a buffer with repeat palette colors 

        //Separated into two buffers (float3 and float2) to avoid needing to use a float8 for each superpixel
        //Still uses 1 extra float per superpixel (float3 == float4)
        std::vector<float> colors_h = CreateSuperPixelColors();
        std::vector<float> positions_h = CreateSuperPixelPositions();

        _algCtx->queue.enqueueWriteBuffer(_algCtx->colors_d, CL_TRUE, 0, colors_h.size() * sizeof(float), colors_h.data());
        _algCtx->queue.enqueueWriteBuffer(_algCtx->positions_d, CL_TRUE, 0, positions_h.size() * sizeof(float), positions_h.data());


        // Change to be non-blocking later

        cl::NDRange global(_pixelizationContext.sourceMat.cols, _pixelizationContext.sourceMat.rows);

        //Lets driver pick size bc im not touching that 
        _algCtx->queue.enqueueNDRangeKernel(_algCtx->kernel, cl::NullRange, global, cl::NullRange);


        // AssociateP2SP(
        //     cl::EnqueueArgs(queue, global), 
        //     _algCtx.source_d, 
        //     _algCtx.colors_d, 
        //     _algCtx.positions_d, 
        //     _algCtx.output_d, 
        //     cl_float2{(float)_pixelizationContext.sourceMat.cols, (float)_pixelizationContext.sourceMat.rows}, 
        //     cl_float2{(float)_inputSettings.wOut, (float)_inputSettings.hOut}, 
        //     _pixelizationContext.differenceCoeff
        // ).wait();
        
        std::vector<uint> output_h(_pixelizationContext.sourceMat.rows * _pixelizationContext.sourceMat.cols);
        _algCtx->queue.enqueueReadBuffer(_algCtx->output_d, CL_TRUE, 0, sizeof(uint) * _pixelizationContext.sourceMat.rows * _pixelizationContext.sourceMat.cols, output_h.data());
        // if(AssociateCount % 10 == 0){
        //     DrawAssociationMap(output_h);
        // }
        AssignPixels(output_h);



        AssociateCount++;
        //Some Algo to process output here
    } 
} 


