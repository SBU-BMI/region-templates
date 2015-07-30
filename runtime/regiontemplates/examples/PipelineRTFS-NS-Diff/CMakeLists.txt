cmake_minimum_required(VERSION 2.6)
project(PipelineRTFS-NS-Diff)

find_package( OpenCV REQUIRED )

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util)
include_directories(${RegionTemplate_SOURCE_DIR})
include_directories(${RegionTemplate_SOURCE_DIR}/Utils)

# nscale dirs
# nscale dirs
find_path( NSCALE_DIR REQUIRED )
set(nscalesrcdir ${NSCALE_DIR}/src/)
set(nscalebuilddir ${NSCALE_DIR}/build/)
#stampede
#set(nscalesrcdir /work/02542/gteodoro/nscale/src/)
#set(nscalebuilddir /work/02542/gteodoro/nscale/build)
#desktop
#set(nscalesrcdir /home/george/nscale/src/)
#set(nscalebuilddir /home/george/nscale/build/)
#notebook
#set(nscalesrcdir /home/george/workspace/nscale-normalization/src/)
#set(nscalebuilddir /home/george/workspace/nscale-normalization/build/)
#laico
#set(nscalesrcdir /home/gteodoro/nscale/src/)
#set(nscalebuilddir /home/gteodoro/nscale/build/)


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
set( libcomponentssrcs NormalizationComp.cpp Segmentation.cpp FeatureExtraction.cpp DiffMaskComp.cpp TaskNormalization.cpp TaskSegmentation.cpp TaskFeatures.cpp TaskDiffMask.cpp)

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
