cmake_minimum_required(VERSION 2.6)
project(PipelineRTFS-NS-Diff)

find_package(OpenCV REQUIRED)

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util)
include_directories(${RegionTemplate_SOURCE_DIR})
include_directories(${RegionTemplate_SOURCE_DIR}/Utils)
include_directories(${RegionTemplate_SOURCE_DIR}/comparativeanalysis)

# nscale dirs
find_path(NSCALE_SRC_DIR REQUIRED)
find_path(NSCALE_BUILD_DIR REQUIRED)
set(nscalesrcdir ${NSCALE_SRC_DIR})
set(nscalebuilddir ${NSCALE_BUILD_DIR})

# nscale includes
include_directories(${nscalesrcdir}/segment)
include_directories(${nscalesrcdir}/segment/cuda)
include_directories(${nscalesrcdir}/common)
include_directories(${nscalesrcdir}/features)
include_directories(${nscalesrcdir}/normalization)

# Autotiling headers
include_directories(${AutoTiling_SOURCE_DIR})
link_directories(${AutoTiling_SOURCE_DIR})

# Find and import OpenSlide
find_path(OPENSLIDE_SRC_DIR REQUIRED)
include_directories(${OPENSLIDE_SRC_DIR})
link_directories(${OPENSLIDE_SRC_DIR}/.libs/)
set(openslide ${OPENSLIDE_SRC_DIR}/.libs/libopenslide.so)

# Make sure the compiler can find the library in runtime system directory
link_directories(${Runtime_SOURCE_DIR})

# nscale lib dir
link_directories(${nscalebuilddir}/lib)

# Create library with the implementation of the stages and tasks
set(libname componentnsdiffdt)

# Source files used by the Runtime System
set(libcomponentssrcs NormalizationComp.cpp Segmentation.cpp 
    FeatureExtraction.cpp TaskNormalization.cpp TaskSegmentation.cpp 
    TaskFeatures.cpp DiffMaskComp.cpp)

# Copy the cache-conf.xml file located in the example folder to the build 
#   folder everytime the program is compiled.
add_custom_target(copyCacheConfdt COMMAND ${CMAKE_COMMAND} 
    -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cache-conf/rtconf.xml 
    ${CMAKE_CURRENT_BINARY_DIR})

# Add rule to generate the Runtime System, as a shared library (.so)
set(libcomparativeanalysis ../../comparativeanalysis/TaskDiffMask.cpp
            ../../comparativeanalysis/pixelcompare/PixelCompare.cpp)
add_library("${libname}" SHARED ${libcomponentssrcs} ${libcomparativeanalysis})

# link libs
target_link_libraries("${libname}" runtimesystem)
target_link_libraries("${libname}" regiontemplates)
# target_link_libraries("${libname}" ${OpenCV_LIBS})
target_link_libraries("${libname}" segment normalization featuresAll)
target_link_libraries("${libname}" openslide)
target_link_libraries("${libname}" autotiling)
        
# Create executable to be built from the files listed
add_executable(PipelineRTFS-NS-Diff-disTiler PipelineManager.cpp)
add_dependencies(PipelineRTFS-NS-Diff-disTiler copyCacheConfdt)

# Link the executable to the runtime system library
target_link_libraries(PipelineRTFS-NS-Diff-disTiler runtimesystem "${libname}")
target_link_libraries(PipelineRTFS-NS-Diff-disTiler regiontemplates)
target_link_libraries(PipelineRTFS-NS-Diff-disTiler autotiling)
