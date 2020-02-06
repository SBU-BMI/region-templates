cmake_minimum_required(VERSION 2.6)
project(IWPP-Halide-disTiler)

find_package(OpenCV REQUIRED)

# add halide
find_path(HALIDE_BUILD_PATH REQUIRED)
include_directories(${HALIDE_BUILD_PATH}/include)
link_directories(${HALIDE_BUILD_PATH}/lib)
# add halide tools
include_directories(${HALIDE_BUILD_PATH}/../tools)
find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

# Creates lib of AutoStage
set(libname autostage)
add_library("${libname}" SHARED AutoStage.cpp)
target_link_libraries("${libname}" runtimesystem)
target_link_libraries("${libname}" regiontemplates)
target_link_libraries("${libname}" Halide)

# Creates executable to be built from the files listed
add_executable(iwpp main.cpp IwppRecon.cpp)
target_link_libraries(iwpp Halide)
target_link_libraries(iwpp ${OpenCV_LIBS})
target_link_libraries(iwpp -ljpeg)
target_link_libraries(iwpp ${PNG_LIBRARY})
target_link_libraries(iwpp "${libname}")
# target_link_libraries(iwpp regiontemplates)
# target_link_libraries(iwpp runtimesystem)




# nscale dirs
find_path(NSCALE_SRC_DIR REQUIRED)
find_path(NSCALE_BUILD_DIR REQUIRED)
set(nscalesrcdir ${NSCALE_SRC_DIR})
set(nscalebuilddir ${NSCALE_BUILD_DIR})

# nscale includes
include_directories(${nscalesrcdir}/segment)
include_directories(${nscalesrcdir}/segment/cuda)
include_directories(${nscalesrcdir}/common)
include_directories(${nscalesrcdir}/features)
include_directories(${nscalesrcdir}/normalization)

# nscale lib dir
link_directories(${nscalebuilddir}/lib)

add_executable(segtest segTest.cpp)
target_link_libraries(segtest ${OpenCV_LIBS})
target_link_libraries(segtest segment normalization)

if(PROFILING)
    add_definitions(-DPROFILING)
endif(PROFILING)

