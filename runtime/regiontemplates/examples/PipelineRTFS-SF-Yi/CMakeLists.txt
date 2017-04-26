cmake_minimum_required(VERSION 2.6)
project(PipelineRTFS-SF-Yi)

find_package( OpenCV REQUIRED )

find_package (ITK)
if (ITK_FOUND)
   include(${ITK_USE_FILE})
endif(ITK_FOUND)

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util)
include_directories(${RegionTemplate_SOURCE_DIR})
include_directories(${RegionTemplate_SOURCE_DIR}/Utils)

# nscale dirs
find_path( NSCALE_SRC_DIR REQUIRED )
find_path( NSCALE_BUILD_DIR REQUIRED )
set(nscalesrcdir ${NSCALE_SRC_DIR})
set(nscalebuilddir ${NSCALE_BUILD_DIR})

#includes
include_directories(${nscalesrcdir}/segment)
include_directories(${nscalesrcdir}/segment/cuda)
include_directories(${nscalesrcdir}/common)
include_directories(${nscalesrcdir}/features)

link_directories(${nscalebuilddir}/lib)

# yis dirs
find_path( YI_SRC_DIR REQUIRED )
find_path( YI_BUILD_DIR REQUIRED )
set(yisrcdir ${YI_SRC_DIR})
set(yibuilddir ${YI_BUILD_DIR})

#includes
include_directories(${yisrcdir}/)

file(COPY ${PipelineRTFS-SF-Yi_SOURCE_DIR}/rtconf.xml DESTINATION ${PipelineRTFS-SF-Yi_BINARY_DIR}/)
# Make sure the compiler can find the library in runtime system directory
link_directories(${Runtime_SOURCE_DIR})

# yis lib dir
link_directories(${yibuilddir}/)

# Create library with components implementation
set(libname componentsrtfsyi)

# Source files used by the Runtime System
set( libcomponentssrcs Segmentation.cpp FeatureExtraction.cpp TaskSegmentation.cpp TaskFeatures.cpp)

# Add rule to generate the Runtime System, as a shared library (.so)
add_library("${libname}" SHARED ${libcomponentssrcs})

#set_target_properties("${libname}" PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}") 
target_link_libraries("${libname}" runtimesystem)
target_link_libraries(${libname} regiontemplates)
target_link_libraries(${libname} ${YI_BUILD_DIR}/liblocalLibraries.a)
target_link_libraries("${libname}" ${OpenCV_LIBS} ${ITK_LIBRARIES})

# nscale libs
target_link_libraries("${libname}" segment featuresAll ${OpenCV_LIBS})
        
# Create executable to be built from the files listed
add_executable(PipelineRTFS-SF-Yi PipelineManager.cpp)

# Link the executable to the runtime system library
target_link_libraries(PipelineRTFS-SF-Yi runtimesystem "${libname}")
target_link_libraries(PipelineRTFS-SF-Yi regiontemplates)
