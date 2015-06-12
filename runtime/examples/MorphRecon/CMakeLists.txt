cmake_minimum_required(VERSION 2.6)
project(MorphRecon)

find_package( OpenCV REQUIRED )
find_package( CUDA 4.0)

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util)

#set(nscaleroot /lustre/medusa/gteodor/hpdc/nscale/)
set(nscaleroot /home/gteodor/nscale/)
include_directories(${nscaleroot}/src/segment)
include_directories(${nscaleroot}/src/segment/cuda)
include_directories(${nscaleroot}/src/features)

# Make sure the compiler can find the library in runtime system directory
link_directories(${Runtime_SOURCE_DIR})
link_directories(${nscaleroot}/lib)

# Create library with components implementation
set(libname reconcomponents)

# Source files used by the Runtime System
set( libcomponentssrcs MophReconComp.cpp TaskTileRecon.cpp TaskFixRecon.cpp ReadInputFileNames.cpp)

# Add rule to generate the Runtime System, as a shared library
add_library("${libname}" SHARED ${libcomponentssrcs})

#set_target_properties("${libname}" PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}") 
target_link_libraries("${libname}" runtimesystem segment featuresAll ${OpenCV_LIBS})
 

# Create executable to be built from the files listed
add_executable(MorphRecon NucleiPipeline.cpp)

# Link the executable to the runtime system library
target_link_libraries(MorphRecon runtimesystem "${libname}")
