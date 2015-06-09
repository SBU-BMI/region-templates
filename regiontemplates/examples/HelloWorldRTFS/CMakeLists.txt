cmake_minimum_required(VERSION 2.6)
project(HelloWorldRTFS)

find_package( OpenCV REQUIRED )

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util)
include_directories(${RegionTemplate_SOURCE_DIR})
include_directories(${RegionTemplate_SOURCE_DIR}/Utils)

# nscale dirs
#stampede
#set(nscalesrcdir /work/02542/gteodoro/nscale/src/)
#set(nscalebuilddir /work/02542/gteodoro/nscale/build)
#desktop
set(nscalesrcdir /home/george/nscale/src/)
set(nscalebuilddir /home/george/nscale/build/)
#notebook
#set(nscalesrcdir /home/george/workspace/nscale-normalization/src/)
#set(nscalebuilddir /home/george/workspace/nscale-normalization/build/)


#/home/george/workspace/nscale-normalization/
#includes
include_directories(${nscalesrcdir}/segment)
include_directories(${nscalesrcdir}/segment/cuda)
include_directories(${nscalesrcdir}/common)
include_directories(${nscalesrcdir}/features)


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
        
# Create executable to be built from the files listed
add_executable(HelloWorldRTFS HelloWorld.cpp)

# Link the executable to the runtime system library
target_link_libraries(HelloWorldRTFS runtimesystem "${libname}")
target_link_libraries(HelloWorldRTFS regiontemplates)

# nscale libs
#target_link_libraries(HelloWorldRT segment featuresAll ${OpenCV_LIBS})
