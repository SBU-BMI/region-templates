cmake_minimum_required(VERSION 2.6)
project(FireDetectionRT)

find_package( OpenCV REQUIRED )

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util)
include_directories(${RegionTemplate_SOURCE_DIR})
include_directories(${RegionTemplate_SOURCE_DIR}/Utils)

# tracking include dir
include_directories(/home/george/fireDetection_C++/code/testVideo/for_redistribution_files_only/)
# matlab dir
include_directories(/usr/local/MATLAB/R2013b/extern/include/)

# Make sure the compiler can find the library in runtime system directory
link_directories(${Runtime_SOURCE_DIR})

# tracking lib dir
link_directories(/home/george/fireDetection_C++/code/testVideo/for_redistribution_files_only/)

# Create library with components implementation
set(libname componentsrt)

# Source files used by the Runtime System
set( libcomponentssrcs FireTrackingComponent.cpp TaskTracking.cpp)

# Add rule to generate the Runtime System, as a shared library (.so)
add_library("${libname}" SHARED ${libcomponentssrcs})

#set_target_properties("${libname}" PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}") 
target_link_libraries("${libname}" runtimesystem)
target_link_libraries(${libname} regiontemplates)

# tracking lib
target_link_libraries("${libname}" testVideo)
        
# Create executable to be built from the files listed
add_executable(FireTrackingRT FireTracking.cpp)

# Link the executable to the runtime system library
target_link_libraries(FireTrackingRT runtimesystem "${libname}")
target_link_libraries(FireTrackingRT regiontemplates)

# nscale libs
#target_link_libraries(HelloWorldRT segment featuresAll ${OpenCV_LIBS})
