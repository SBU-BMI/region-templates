cmake_minimum_required(VERSION 2.6)
project(HelloWorldRT)

find_package( OpenCV REQUIRED )

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util)
include_directories(${RegionTemplate_SOURCE_DIR})
include_directories(${RegionTemplate_SOURCE_DIR}/Utils)

# nscale dirs
#/lustre/medusa/gteodor/nscale/build

#include_directories(/Users/george/Documents/emory/nscale-sc13/nscale/src/segment)
#include_directories(/Users/george/Documents/emory/nscale-sc13/nscale/src/segment/cuda)
#include_directories(/Users/george/Documents/emory/nscale-sc13/nscale/src/common)
#include_directories(/Users/george/Documents/emory/nscale-sc13/nscale/src/features)
include_directories(/lustre/medusa/gteodor/nscale/src/segment)
include_directories(/lustre/medusa/gteodor/nscale/src/segment/cuda)
include_directories(/lustre/medusa/gteodor/nscale/src/common)
include_directories(/lustre/medusa/gteodor/nscale/src/features)



# Make sure the compiler can find the library in runtime system directory
link_directories(${Runtime_SOURCE_DIR})
# nscale lib dir
#link_directories(/Users/george/Documents/emory/nscale-sc13/nscale/lib)
link_directories(/lustre/medusa/gteodor/nscale/build/lib)
# Create library with components implementation
set(libname componentsrt)

# Source files used by the Runtime System
set( libcomponentssrcs Segmentation.cpp FeatureExtraction.cpp TaskSegmentation.cpp TaskFeatures.cpp)

# Add rule to generate the Runtime System, as a shared library (.so)
add_library("${libname}" SHARED ${libcomponentssrcs})

#set_target_properties("${libname}" PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}") 
target_link_libraries("${libname}" runtimesystem)
target_link_libraries(${libname} regiontemplates)

# nscale libs
target_link_libraries("${libname}" segment featuresAll ${OpenCV_LIBS})
        
# Create executable to be built from the files listed
add_executable(HelloWorldRT HelloWorld.cpp)

# Link the executable to the runtime system library
target_link_libraries(HelloWorldRT runtimesystem "${libname}")
target_link_libraries(HelloWorldRT regiontemplates)

# nscale libs
#target_link_libraries(HelloWorldRT segment featuresAll ${OpenCV_LIBS})
