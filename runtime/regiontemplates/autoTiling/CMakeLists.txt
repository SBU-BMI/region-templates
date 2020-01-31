cmake_minimum_required(VERSION 2.6)
project(AutoTiling)

# OpenCV
find_package(OpenCV REQUIRED)

# Find and generate an OpenSlide lib
find_path(OPENSLIDE_SRC_DIR REQUIRED)
include_directories(${OPENSLIDE_SRC_DIR})
link_directories(${OPENSLIDE_SRC_DIR}/.libs/)
set(openslide ${OPENSLIDE_SRC_DIR}/.libs/libopenslide.so)

# add halide
find_path(HALIDE_BUILD_PATH REQUIRED)
include_directories(${HALIDE_BUILD_PATH}/include)
link_directories(${HALIDE_BUILD_PATH}/lib)
# add halide tools
include_directories(${HALIDE_BUILD_PATH}/../tools)
find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

# Source files
set(libautotilingsrc TiledRTCollection.cpp RegTiledRTCollection.cpp 
    IrregTiledRTCollection.cpp BGPreTiledRTCollection.cpp 
    HybridDenseTiledRTCollection.cpp 
    CostFunction.cpp costFuncs/OracleCostFunction.cpp 
    costFuncs/PropagateDistCostFunction.cpp
    costFuncs/ThresholdBGMasker.cpp costFuncs/ThresholdBGCostFunction.cpp 
    svs/svsUtils.cpp tilingAlgs/tilingUtil.cpp tilingAlgs/denseFromBG.cpp 
    tilingAlgs/listCutting.cpp tilingAlgs/kdTreeCutting.cpp 
    tilingAlgs/quadTreeCutting.cpp)

# Creates autotiling lib
set(libname autotiling)
add_library("${libname}" SHARED ${libautotilingsrc})
include_directories("./svs")
include_directories("./costFuncs")
include_directories("./tilingAlgs")

target_link_libraries("${libname}" ${OpenCV_LIBS})
target_link_libraries("${libname}" openslide)

if(PROFILING)
    add_definitions(-DPROFILING)
endif(PROFILING)
