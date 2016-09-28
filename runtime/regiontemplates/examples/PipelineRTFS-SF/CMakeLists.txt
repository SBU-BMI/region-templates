cmake_minimum_required(VERSION 2.6)
project(PipelineRTFS-SF)

find_package( OpenCV REQUIRED )

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

file(COPY ${PipelineRTFS-SF_SOURCE_DIR}/rtconf.xml DESTINATION ${PipelineRTFS-SF_BINARY_DIR}/)
# Make sure the compiler can find the library in runtime system directory
link_directories(${Runtime_SOURCE_DIR})
# nscale lib dir
#link_directories(/Users/george/Documents/emory/nscale-sc13/nscale/lib)
link_directories(${nscalebuilddir}/lib)
# Create library with components implementation
set(libname componentsrtfs)

# Source files used by the Runtime System
set( libcomponentssrcs Segmentation.cpp FeatureExtraction.cpp TaskSegmentation.cpp TaskFeatures.cpp)

# Add rule to generate the Runtime System, as a shared library (.so)
add_library("${libname}" SHARED ${libcomponentssrcs})

#set_target_properties("${libname}" PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}") 
target_link_libraries("${libname}" runtimesystem)
target_link_libraries(${libname} regiontemplates)

# nscale libs
target_link_libraries("${libname}" segment featuresAll ${OpenCV_LIBS})
       
if(USE_ADIOS)
    set(ADIOS_ROOT $ENV{ADIOS_DIR})
    set(ADIOS_DIR $ENV{ADIOS_DIR})
    set(ADIOS_USE_STATIC_LIBS OFF)
    LIST(APPEND CMAKE_MODULE_PATH "${ADIOS_DIR}/etc")
    LIST(APPEND CMAKE_FIND_ROOT_PATH "${ADIOS_DIR}")
    find_package (ADIOS COMPONENTS REQUIRED)

    include_directories(${ADIOS_DIR}/include/)
endif(USE_ADIOS)

 
# Create executable to be built from the files listed
add_executable(PipelineRTFS-SF PipelineManager.cpp)

# Link the executable to the runtime system library
target_link_libraries(PipelineRTFS-SF runtimesystem "${libname}")
target_link_libraries(PipelineRTFS-SF regiontemplates)
target_link_libraries(PipelineRTFS-SF ${ADIOS_LIBRARIES})
