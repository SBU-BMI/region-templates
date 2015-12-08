cmake_minimum_required(VERSION 2.6)
project(PipelineRTFS-NS-Diff-ParStudy)

find_package( OpenCV REQUIRED )

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util)
include_directories(${RegionTemplate_SOURCE_DIR})
include_directories(${RegionTemplate_SOURCE_DIR}/Utils)

# nscale dirs
find_path( NSCALE_DIR REQUIRED )
set(nscalesrcdir ${NSCALE_DIR}/src/)
set(nscalebuilddir ${NSCALE_DIR}/build/)

#includes
include_directories(${nscalesrcdir}/segment)
include_directories(${nscalesrcdir}/segment/cuda)
include_directories(${nscalesrcdir}/common)
include_directories(${nscalesrcdir}/features)
include_directories(${nscalesrcdir}/normalization)

file(COPY ${PipelineRTFS-NS-Diff-ParStudy_SOURCE_DIR}/rtconf.xml DESTINATION ${PipelineRTFS-NS-Diff-ParStudy_BINARY_DIR}/)

# Make sure the compiler can find the library in runtime system directory
link_directories(${Runtime_SOURCE_DIR})
# nscale lib dir
link_directories(${nscalebuilddir}/lib)
# Create library with components implementation
set(libname componentnsdiffstudy)

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
add_executable(PipelineRTFS-NS-Diff-ParStudy PipelineManager.cpp)

# Link the executable to the runtime system library
target_link_libraries(PipelineRTFS-NS-Diff-ParStudy runtimesystem "${libname}")
target_link_libraries(PipelineRTFS-NS-Diff-ParStudy regiontemplates)
