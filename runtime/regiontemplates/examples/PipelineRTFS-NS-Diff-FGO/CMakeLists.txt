cmake_minimum_required(VERSION 2.6)
project(PipelineRTFS-NS-Diff-FGO)

add_definitions( -std=c++11 -ggdb -gdwarf-2 -fopenmp)
#add_definitions( -std=c++11 )

find_package( OpenCV REQUIRED )

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util)
include_directories(${RegionTemplate_SOURCE_DIR})
include_directories(${RegionTemplate_SOURCE_DIR}/Utils)
include_directories(${RegionTemplate_SOURCE_DIR}/comparativeanalysis)

# nscale dirs
find_path(NSCALE_SRC_DIR REQUIRED)
find_path(NSCALE_BUILD_DIR REQUIRED)
set(nscalesrcdir ${NSCALE_SRC_DIR})
set(nscalebuilddir ${NSCALE_BUILD_DIR})


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
set(libname componentnsdifffgo)

# Source files used by the Runtime System
set(libcomponentssrcs NormalizationComp.cpp Segmentation.cpp FeatureExtraction.cpp
         TaskFeatures.cpp DiffMaskComp.cpp jsoncpp.cpp graph/min_cut.cpp graph/reuse_tree.cpp
         merging.cpp fgm.cpp cutting_algorithms.cpp)

set(libcomparativeanalysis ../../comparativeanalysis/TaskDiffMask.cpp
            ../../comparativeanalysis/pixelcompare/PixelCompare.cpp)

#Copy the cache-conf.xml file located in the example folder to the build folder everytime the program is compiled.
add_custom_target(copyCacheConfToPipelineRTFS-NS-Diff-FGO COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cache-conf/rtconf.xml ${CMAKE_CURRENT_BINARY_DIR})

# Add rule to generate the Runtime System, as a shared library (.so)
add_library("${libname}" SHARED ${libcomponentssrcs} ${libcomparativeanalysis})

#set_target_properties("${libname}" PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}") 
target_link_libraries("${libname}" runtimesystem)
target_link_libraries(${libname} regiontemplates)

# nscale libs
target_link_libraries("${libname}" segment normalization featuresAll ${OpenCV_LIBS})
        
# Create executable to be built from the files listed
add_executable(PipelineRTFS-NS-Diff-FGO PipelineManager.cpp)
add_dependencies(PipelineRTFS-NS-Diff-FGO copyCacheConfToPipelineRTFS-NS-Diff-FGO)

# Link the executable to the runtime system library
target_link_libraries(PipelineRTFS-NS-Diff-FGO runtimesystem "${libname}")
target_link_libraries(PipelineRTFS-NS-Diff-FGO regiontemplates)
