include("C:/Users/cojac/PixelQT/build/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Pixelizet-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "C:/Users/cojac/PixelQT/build/Pixelizet.exe"
    GENERATE_QT_CONF
)
