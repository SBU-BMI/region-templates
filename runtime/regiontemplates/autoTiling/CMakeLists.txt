cmake_minimum_required(VERSION 2.6)
project(AutoTiling)

# OpenCV
find_package(OpenCV REQUIRED)

# Find and generate an OpenSlide lib
find_path(OPENSLIDE_SRC_DIR REQUIRED)
include_directories(${OPENSLIDE_SRC_DIR})
link_directories(${OPENSLIDE_SRC_DIR}/.libs/)
set(openslide ${OPENSLIDE_SRC_DIR}/.libs/libopenslide.so)

# Source files
# set(libautotilingsrc TiledRTCollection.cpp RegTiledRTCollection.cpp 
#     IrregTiledRTCollection.cpp costFuncs/ThresholdBGMasker.cpp 
#     svs/svsUtils.cpp RTCollectionTilingPipeline.cpp)
set(libautotilingsrc svs/svsUtils.cpp RTCollectionTilingPipeline.cpp)

# Creates autotiling lib
set(libname autotiling)
add_library("${libname}" SHARED ${libautotilingsrc})
include_directories("./svs")

target_link_libraries("${libname}" ${OpenCV_LIBS})
target_link_libraries("${libname}" openslide)
