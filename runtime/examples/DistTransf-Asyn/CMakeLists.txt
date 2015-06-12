cmake_minimum_required(VERSION 2.6)
project(DistTransf-Asyn)

find_package( OpenCV REQUIRED )
find_package( CUDA 4.0)

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util)

set(nscaleroot /home/gteodor/nscale)
include_directories(${nscaleroot}/src/segment)
include_directories(${nscaleroot}/src/segment/cuda)
include_directories(${nscaleroot}/src/features)

# Make sure the compiler can find the library in runtime system directory
link_directories(${Runtime_SOURCE_DIR})
link_directories(${nscaleroot}/lib)

# Create library with components implementation
set(libname distasyncomponents)

# Source files used by the Runtime System
set(libcomponentssrcs TaskArgumentMat.cpp DistComp.cpp TaskTileProp.cpp TaskFixProp.cpp ReadInputFileNames.cpp)

# Add rule to generate the Runtime System, as a shared library
add_library("${libname}" SHARED ${libcomponentssrcs})

#set_target_properties("${libname}" PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}") 
target_link_libraries("${libname}" runtimesystem segment featuresAll ${OpenCV_LIBS})
 

# Create executable to be built from the files listed
add_executable(DistTransf-Asyn DistPipeline.cpp)

# Link the executable to the runtime system library
target_link_libraries(DistTransf-Asyn runtimesystem "${libname}")
