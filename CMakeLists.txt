cmake_minimum_required(VERSION 3.16)

set(CMAKE_MODULE_PATH "${CMAKE_ROOT}/Modules" ${CMAKE_MODULE_PATH})

if (WIN32)
    project(Pixelizet LANGUAGES CXX)
elseif (UNIX)
    project(Pixelizet)
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

add_subdirectory("${CMAKE_SOURCE_DIR}/Pixelizer")

#QT inclusion
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()


#Inlcudes
set(FORMS_DIR "${CMAKE_SOURCE_DIR}/forms")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")

include_directories(${FORMS_DIR})
include_directories(${INCLUDE_DIR})
include_directories((${SOURCE_DIR}))

file(GLOB_RECURSE SOURCES
    "${FORMS_DIR}/*.ui"
    "${FORMS_DIR}/*.qrc"
    "${INCLUDE_DIR}/*.h"
    "${SOURCE_DIR}/*.cpp"
)

#Setup Debug
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
    $<$<CONFIG:RELWITHDEBINFO>:QT_MESSAGELOGCONTEXT>
)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${FORMS_DIR})

qt_add_executable(Pixelizet WIN32 MACOSX_BUNDLE ${SOURCES})
qt_add_resources(Pixelizet "images"
    PREFIX "/"
    FILES forms/images/HelloMario.png
)

add_custom_command(TARGET Pixelizet POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:Pixelizet>
        $<TARGET_FILE_DIR:Pixelizet>
    COMMAND_EXPAND_LISTS
)


# add_custom_command(TARGET Pixelizet POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#         "C:/"
#         $<TARGET_FILE_DIR:Pixelizet>
#     COMMAND_EXPAND_LISTS
# )

target_link_libraries(Pixelizet PRIVATE Pixelizer Qt6::Widgets Qt6::Core)

include(GNUInstallDirs)

install(TARGETS Pixelizet
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)


target_include_directories(Pixelizet PRIVATE ${FORMS_DIR})
target_include_directories(Pixelizet PRIVATE ${INCLUDE_DIR})
target_include_directories(Pixelizet PRIVATE ${SOURCE_DIR})


qt_generate_deploy_app_script(
    TARGET Pixelizet
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})