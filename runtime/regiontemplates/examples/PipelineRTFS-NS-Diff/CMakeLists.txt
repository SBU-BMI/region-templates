cmake_minimum_required(VERSION 2.6)
project(PipelineRTFS-NS-Diff)

find_package( OpenCV REQUIRED )

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


#includes
include_directories(${nscalesrcdir}/segment)
include_directories(${nscalesrcdir}/segment/cuda)
include_directories(${nscalesrcdir}/common)
include_directories(${nscalesrcdir}/features)
include_directories(${nscalesrcdir}/normalization)

# Make sure the compiler can find the library in runtime system directory
link_directories(${Runtime_SOURCE_DIR})
# nscale lib dir
#link_directories(/Users/george/Documents/emory/nscale-sc13/nscale/lib)
link_directories(${nscalebuilddir}/lib)
# Create library with components implementation
set(libname componentnsdiff)

# Source files used by the Runtime System
set(libcomponentssrcs NormalizationComp.cpp Segmentation.cpp FeatureExtraction.cpp
        ../../comparativeanalysis/DiffMaskComp.cpp TaskNormalization.cpp TaskSegmentation.cpp TaskFeatures.cpp
        ../../comparativeanalysis/TaskDiffMask.cpp ../../comparativeanalysis/pixelcompare/PixelCompare.cpp)

#set( libcomponentssrcs NormalizationComp.cpp Segmentation.cpp FeatureExtraction.cpp ../../comparativeanalysis/DiffMaskComp.cpp TaskNormalization.cpp TaskSegmentation.cpp TaskFeatures.cpp ../PipelineRTFS-NS-Diff-AH/TaskDiffMask.cpp)

# Add rule to generate the Runtime System, as a shared library (.so)
add_library("${libname}" SHARED ${libcomponentssrcs})

#set_target_properties("${libname}" PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}") 
target_link_libraries("${libname}" runtimesystem)
target_link_libraries(${libname} regiontemplates)

# nscale libs
target_link_libraries("${libname}" segment normalization featuresAll ${OpenCV_LIBS})
        
# Create executable to be built from the files listed
add_executable(PipelineRTFS-NS-Diff PipelineManager.cpp)

# Link the executable to the runtime system library
target_link_libraries(PipelineRTFS-NS-Diff runtimesystem "${libname}")
target_link_libraries(PipelineRTFS-NS-Diff regiontemplates)
