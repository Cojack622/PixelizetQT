# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\Pixelizet_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Pixelizet_autogen.dir\\ParseCache.txt"
  "Pixelizet_autogen"
  )
endif()
