#include "Pixelizer.hpp"
#define CL_HPP_TARGET_OPENCL_VERSION 300
#include <CL/opencl.hpp>

#include <fstream>
#include <optional>

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

        for(size_t i = 0; i < associations.size(); i++) {
            Pixel p( 
                *(_pixelizationContext.sourceMat.ptr<cv::Vec3f>(i / _pixelizationContext.sourceMat.cols, i % _pixelizationContext.sourceMat.cols)),
                cv::Point2i(i % _pixelizationContext.sourceMat.cols, i / _pixelizationContext.sourceMat.cols)
            );
            _pixelizationContext.superPixels[associations[i]].addPixel(std::move(p));
        }
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
        void kernel AssociatePixelsToSuperPixel(global const float3* sourceMat, global const float3* superPixelColors, global const float2* superPixelPositions, global uint* associations, float2 N, float2 M, float diffCoeff) {
            int cIn = get_global_id(0);
            int rIn = get_global_id(1);


            int flatIdx = cIn + rIn * (int)N.x;

            //Ensure non-encroachment
            if(rIn < N.y && cIn < N.x) {
                float3 dPixel = sourceMat[flatIdx];

                float minCost = INFINITY;
                uint minSuperPixelIdx = 0;
                for(int i = 0; i < M.x * M.y; i++) {
                    float3 colorDiff = superPixelColors[i] - dPixel;
                    float2 posDiff = superPixelPositions[i] - convert_float2((int2)(cIn, rIn));

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
        cl::Buffer source_d(context, CL_MEM_READ_ONLY, /*sizeof(_pixelizationContext.sourceMat.data)*/ _pixelizationContext.sourceMat.rows * _pixelizationContext.sourceMat.cols * sizeof(cl_float4));
        // This could be an image buffer since its not in LAB, but idk
        cl::Buffer output_d(context, CL_MEM_WRITE_ONLY, sizeof(uint) * _inputSettings.hOut * _inputSettings.wOut);


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


        // MOVE CONSTANT INPUT/OUTPUT TO DEDICATED STARTUP FUNCTION since it only needs to be done once
        // Not sure if it will copy the data or simply point to it 

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
        
        std::vector<uint> output_h(_inputSettings.hOut * _inputSettings.wOut);
        _algCtx->queue.enqueueReadBuffer(_algCtx->output_d, CL_TRUE, 0, sizeof(uint) * _inputSettings.hOut * _inputSettings.wOut, output_h.data());

        AssignPixels(output_h);
        //Some Algo to process output here
    } 
} 


