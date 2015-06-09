cmake_minimum_required(VERSION 2.6)
project(NucleiPipeline-Bundle)

find_package( OpenCV REQUIRED )
find_package( CUDA 5.0)

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util)

set(NSCALE_SRC_DIR "" CACHE FILEPATH "Path to nscale root src dir")
set(NSCALE_BUILD_DIR "" CACHE FILEPATH "Path to nscale root build dir")

if(NOT EXISTS ${NSCALE_SRC_DIR}/src/segment/ConnComponents.h)
  message(SEND_ERROR "Can't find ConnComponents.h in
${NSCALE_SRC_DIR}/src/segment/ConnComponents.h! PLEASE, CONFIGURE
NSCALE_SRC_DIR!!!")
endif()

if(NOT EXISTS ${NSCALE_BUILD_DIR}/lib/libsegment.so)
  message(SEND_ERROR "Can't find libsegment.so in
${NSCALE_BUILD_DIR}/lib/libsegment.so. PLEASE, CONFIGURE NSCALE_BUILD_DIR!!!")
endif()

#set(nscaleroot /home/gteodor/nscale)

include_directories(${NSCALE_SRC_DIR}/src/common)
include_directories(${NSCALE_SRC_DIR}/src/segment)
include_directories(${NSCALE_SRC_DIR}/src/segment/cuda)
include_directories(${NSCALE_SRC_DIR}/src/features)

# Make sure the compiler can find the library in runtime system directory
link_directories(${Runtime_SOURCE_DIR})
link_directories(${NSCALE_BUILD_DIR}/lib)

# Create library with components implementation
set(libname npipecomponentsbundle)

# Source files used by the Runtime System
set( libcomponentssrcs CompSeg.cpp TaskDoAll.cpp)

# Add rule to generate the Runtime System, as a shared library
add_library("${libname}" SHARED ${libcomponentssrcs})

#set_target_properties("${libname}" PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}") 
target_link_libraries("${libname}" cci_common runtimesystem segment featuresAll ${OpenCV_LIBS})
 

# Create executable to be built from the files listed
add_executable(NucleiPipelineBundle NucleiPipeline.cpp ReadInputFileNames.cpp)

# Link the executable to the runtime system library
target_link_libraries(NucleiPipelineBundle runtimesystem "${libname}" cci_common)
