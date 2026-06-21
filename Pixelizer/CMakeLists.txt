
set(PIXELIZER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Pixelizer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Pixelizer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PixelizerLog.hpp"
)

#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
option(ENABLE_OPENCL "Enable OpenCL support" ON)

message(STATUS "OpenCL directory: ${OpenCL_INCLUDE_DIR}")

find_package(OpenCV REQUIRED)

if(NOT APPLE)
    message(STATUS "Checking for OpenCL...")
    find_package(OpenCL)
    #find_package(OpenCLHeaders REQUIRED)
    #find_package(OpenCLHeadersCpp REQUIRED)
endif()


#Later on, change to include 
if(OpenCL_FOUND)
    message(STATUS "OpenCL found! Linking...")
    
    list(APPEND PIXELIZER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/PixelizerOpenCL.cpp")
    
    #target_link_libraries(Pixelizer PUBLIC ${OpenCL_LIBRARIES})
else()
    message(STATUS "OpenCL not found! Linking...")

    list(APPEND PIXELIZER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/PixelizerPThread.cpp")
endif()

add_library(Pixelizer STATIC
    ${PIXELIZER_SOURCES}
)

#Later on, change to include 
if(OpenCL_FOUND)

    target_compile_definitions(Pixelizer PUBLIC PIXELIZER_USE_OPENCL)


    target_include_directories(Pixelizer
        PUBLIC
        ${OpenCL_INCLUDE_DIR}
    )

    if(WIN32)
        get_filename_component(OpenCL_LIB_DIR "${OpenCL_LIBRARIES}" DIRECTORY)

        find_file(OpenCL_DLL 
            NAMES OpenCL.dll 
            HINTS 
                "${OpenCL_LIB_DIR}"
                "${OpenCL_LIB_DIR}/../bin"
                "${OpenCL_LIB_DIR}/../../bin"
            REQUIRED
        )
        message(STATUS "OpenCL DLL found: ${OpenCL_DLL}")

        add_library(OpenCL::OpenCL_Runtime SHARED IMPORTED)
        set_target_properties(OpenCL::OpenCL_Runtime PROPERTIES
            IMPORTED_IMPLIB "${OpenCL_LIBRARIES}"
            IMPORTED_LOCATION "${OpenCL_DLL}"
        )
        
        target_link_libraries(Pixelizer PUBLIC OpenCL::OpenCL_Runtime)

    endif()
endif()


include(GenerateExportHeader)
generate_export_header(Pixelizer
    BASE_NAME PIX
    EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/Pixelizer_export.h"
)

target_include_directories(Pixelizer
    PUBLIC 
    "${CMAKE_CURRENT_BINARY_DIR}"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:include>"
    ${OpenCV_INCLUDE_DIRS}
)

if(OpenCL_FOUND)
    target_include_directories(Pixelizer
        PUBLIC
        ${OpenCL_INCLUDE_DIR}
        ${OpenCL_HEADERS}
    )
endif()

target_link_libraries(Pixelizer PUBLIC ${OpenCV_LIBS})

#target_compile_options(Pixelizer PRIVATE -H)