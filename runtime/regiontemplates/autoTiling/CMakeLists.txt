cmake_minimum_required(VERSION 2.6)
project(AutoTiling)

# OpenCV
find_package(OpenCV REQUIRED)

# add openmp
find_package(OpenMP REQUIRED)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

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
    HybridDenseTiledRTCollection.cpp costFuncs/MultiObjCostFunction.cpp
    CostFunction.cpp costFuncs/OracleCostFunction.cpp 
    costFuncs/PropagateDistCostFunction.cpp
    costFuncs/PrePartThresBGCostFunction.cpp
    costFuncs/ThresholdBGMasker.cpp costFuncs/ColorThresholdBGMasker.cpp 
    costFuncs/ThresholdBGCostFunction.cpp tilingAlgs/fineBgRemoval.cpp 
    svs/svsUtils.cpp tilingAlgs/tilingUtil.cpp tilingAlgs/denseFromBG.cpp 
    tilingAlgs/listCutting.cpp tilingAlgs/bgRemListCutting.cpp 
    tilingAlgs/kdTreeCutting.cpp tilingAlgs/quadTreeCutting.cpp 
    tilingAlgs/fixedGrid.cpp)

# Creates autotiling lib
set(libname autotiling)
add_library("${libname}" SHARED ${libautotilingsrc})
include_directories("./svs")
include_directories("./costFuncs")
include_directories("./tilingAlgs")

target_link_libraries("${libname}" ${OpenCV_LIBS})
target_link_libraries("${libname}" openslide)
target_compile_options("${libname}" PRIVATE -lopenmp)


if(PROFILING)
    add_definitions(-DPROFILING)
endif(PROFILING)
