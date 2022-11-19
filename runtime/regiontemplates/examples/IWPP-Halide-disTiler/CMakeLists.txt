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
set(autostage)
add_library(autostage SHARED AutoStage.cpp)
target_link_libraries(autostage runtimesystem)
target_link_libraries(autostage regiontemplates)
target_link_libraries(autostage Halide)

# # Creates lib of halide iwpp
# set(halideiwpp)
# add_library(halideiwpp SHARED HalideIwpp.cpp)
# target_link_libraries(halideiwpp runtimesystem)
# target_link_libraries(halideiwpp regiontemplates)
# target_link_libraries(halideiwpp Halide)
# target_link_libraries(halideiwpp ${OpenCV_LIBS})


# LightGBM
include_directories(/home/willian/git/LightGBM/include)
add_library(lightgbm SHARED IMPORTED)
set_target_properties(lightgbm PROPERTIES IMPORTED_LOCATION /home/willian/git/LightGBM/lib_lightgbm.so)

# Find and generate an OpenSlide lib
find_path(OPENSLIDE_SRC_DIR REQUIRED)
include_directories(${OPENSLIDE_SRC_DIR})
link_directories(${OPENSLIDE_SRC_DIR}/.libs/)
set(openslide ${OPENSLIDE_SRC_DIR}/.libs/libopenslide.so)

add_executable(lgb_test lgb_test.cpp)
target_link_libraries(lgb_test lightgbm)
target_link_libraries(lgb_test openslide)
target_link_libraries(lgb_test ${OpenCV_LIBS})

# Creates executable to be built from the files listed
add_executable(iwpp main.cpp pipeline1.cpp misc.cpp HalideIwpp.cpp)
target_link_libraries(iwpp Halide)
target_link_libraries(iwpp ${OpenCV_LIBS})
target_link_libraries(iwpp -ljpeg)
target_link_libraries(iwpp ${PNG_LIBRARY})
target_link_libraries(iwpp regiontemplates)
target_link_libraries(iwpp runtimesystem)
target_link_libraries(iwpp autostage)

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

################### TESTS

# # Create GPU testing executable
# add_executable(hgpu cudaSumTest.cpp)
# target_link_libraries(hgpu Halide)
# target_link_libraries(hgpu ${OpenCV_LIBS})

# # Segmentation reference of results
# add_executable(segtest segTest.cpp)
# target_link_libraries(segtest ${OpenCV_LIBS})
# target_link_libraries(segtest segment normalization)

if(PROFILING)
    add_definitions(-DPROFILING)
endif(PROFILING)

