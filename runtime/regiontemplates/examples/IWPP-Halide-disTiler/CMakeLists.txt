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

# target_link_libraries(iwpp `libpng-config --cflags --ldflags`)
# add_dependencies(PipelineRTFS-NS-Diff-disTiler copyCacheConfdt)






# # Make sure the compiler can find the includes of the runtime system 
# include_directories(${Runtime_SOURCE_DIR})
# include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
# include_directories(${Runtime_SOURCE_DIR}/util)
# include_directories(${RegionTemplate_SOURCE_DIR})
# include_directories(${RegionTemplate_SOURCE_DIR}/Utils)
# include_directories(${RegionTemplate_SOURCE_DIR}/comparativeanalysis)

# # nscale dirs
# find_path(NSCALE_SRC_DIR REQUIRED)
# find_path(NSCALE_BUILD_DIR REQUIRED)
# set(nscalesrcdir ${NSCALE_SRC_DIR})
# set(nscalebuilddir ${NSCALE_BUILD_DIR})

# # nscale includes
# include_directories(${nscalesrcdir}/segment)
# include_directories(${nscalesrcdir}/segment/cuda)
# include_directories(${nscalesrcdir}/common)
# include_directories(${nscalesrcdir}/features)
# include_directories(${nscalesrcdir}/normalization)

# # Autotiling headers
# include_directories(${AutoTiling_SOURCE_DIR})
# link_directories(${AutoTiling_SOURCE_DIR})

# # Find and import OpenSlide
# find_path(OPENSLIDE_SRC_DIR REQUIRED)
# include_directories(${OPENSLIDE_SRC_DIR})
# link_directories(${OPENSLIDE_SRC_DIR}/.libs/)
# set(openslide ${OPENSLIDE_SRC_DIR}/.libs/libopenslide.so)

# # Make sure the compiler can find the library in runtime system directory
# link_directories(${Runtime_SOURCE_DIR})

# # nscale lib dir
# link_directories(${nscalebuilddir}/lib)

# # Create library with the implementation of the stages and tasks
# set(libname componentnsdiffdt)

# # Source files used by the Runtime System
# set(libcomponentssrcs NormalizationComp.cpp Segmentation.cpp 
#     FeatureExtraction.cpp TaskNormalization.cpp TaskSegmentation.cpp 
#     TaskFeatures.cpp DiffMaskComp.cpp)

# # Copy the cache-conf.xml file located in the example folder to the build 
# #   folder everytime the program is compiled.
# add_custom_target(copyCacheConfdt COMMAND ${CMAKE_COMMAND} 
#     -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cache-conf/rtconf.xml 
#     ${CMAKE_CURRENT_BINARY_DIR})

# # Add rule to generate the Runtime System, as a shared library (.so)
# set(libcomparativeanalysis ../../comparativeanalysis/TaskDiffMask.cpp
#             ../../comparativeanalysis/pixelcompare/PixelCompare.cpp)
# add_library("${libname}" SHARED ${libcomponentssrcs} ${libcomparativeanalysis})

# # link libs
# target_link_libraries("${libname}" runtimesystem)
# target_link_libraries("${libname}" regiontemplates)
# # target_link_libraries("${libname}" ${OpenCV_LIBS})
# target_link_libraries("${libname}" segment normalization featuresAll)
# target_link_libraries("${libname}" openslide)
# target_link_libraries("${libname}" autotiling)
        
# # Create executable to be built from the files listed
# add_executable(PipelineRTFS-NS-Diff-disTiler PipelineManager.cpp)
# add_dependencies(PipelineRTFS-NS-Diff-disTiler copyCacheConfdt)

# # Link the executable to the runtime system library
# target_link_libraries(PipelineRTFS-NS-Diff-disTiler runtimesystem "${libname}")
# target_link_libraries(PipelineRTFS-NS-Diff-disTiler regiontemplates)
# target_link_libraries(PipelineRTFS-NS-Diff-disTiler autotiling)














# cmake_minimum_required(VERSION 2.8.9)
# project (disTiler-halide)
# add_definitions(-std=c++11)

# # add opencv
# find_package(OpenCV REQUIRED)
# include_directories(${OpenCV_INCLUDE_DIRS})

# # add mpi
# find_package(MPI REQUIRED)
# include_directories(SYSTEM ${MPI_INCLUDE_PATH})

# # add pthreads
# find_package(Threads REQUIRED)

# # add openmp
# find_package(OpenMP REQUIRED)
# if (OPENMP_FOUND)
#     set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
#     set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
#     set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
# endif()

# # add halide
# find_path(HALIDE_BUILD_PATH REQUIRED)
# include_directories(${HALIDE_BUILD_PATH}/include)
# link_directories(${HALIDE_BUILD_PATH}/lib)

# # create blur jit lib
# set(blurjit blurjit)
# set(blurjitsrcs BlurJIT.cpp)
# add_library("${blurjit}" STATIC ${blurjitsrcs})
# target_include_directories("${blurjit}" PUBLIC .)
# target_link_libraries(${blurjit} ${OpenCV_LIBS})

# # create blur aot generator executable
# add_executable(libblur ${HALIDE_BUILD_PATH}/../tools/GenGen.cpp BlurAOT.cpp)
# set_target_properties(libblur PROPERTIES LINKER_LANGUAGE CXX)
# set_target_properties(libblur PROPERTIES ENABLE_EXPORTS ON)
# target_link_libraries(libblur Halide)
# target_compile_options(libblur PRIVATE -fno-rtti)
# target_compile_options(libblur PRIVATE -pthread)
# target_compile_options(libblur PRIVATE -lHalide)

# # make target to generate bluraot lib and clean it
# add_custom_target(
#     genbluraot
#     COMMAND ./libblur -g blurAOT -o . target=host
#     DEPENDS libblur)
# set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
#    "${CMAKE_CURRENT_BINARY_DIR}/blurAOT.a"
#    "${CMAKE_CURRENT_BINARY_DIR}/blurAOT.h")


# add_library(libbluraot STATIC IMPORTED)
# set_target_properties(libbluraot PROPERTIES 
#     IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/blurAOT.a
#     DEPENDS genbluraot)

# # create priority queue lib
# set(priorityq priorityq)
# set(priorityqsrcs PriorityQ.h)
# add_library("${priorityq}" STATIC ${priorityqsrcs})
# set_target_properties(priorityq PROPERTIES LINKER_LANGUAGE CXX)
# target_include_directories("${priorityq}" PUBLIC .)
# target_link_libraries(${priorityq} ${OpenCV_LIBS})

# # create distTiling lib
# set(distiler distiler)
# set(distilersrcs distTiling.cpp)
# add_library("${distiler}" STATIC ${distilersrcs})
# set_target_properties(distiler PROPERTIES LINKER_LANGUAGE CXX)
# target_compile_options(distiler PRIVATE -pthread)
# target_compile_options(distiler PRIVATE 
#     -include "${CMAKE_CURRENT_BINARY_DIR}/blurAOT.h")
# add_dependencies(distiler genbluraot)
# target_include_directories("${distiler}" PUBLIC .)
# target_link_libraries(distiler ${CMAKE_THREAD_LIBS_INIT})
# target_link_libraries(distiler ${OpenCV_LIBS})
# target_link_libraries(distiler ${MPI_CXX_LIBRARIES})
# target_link_libraries(distiler Halide)
# target_link_libraries(distiler libbluraot -ldl)
# target_link_libraries(distiler priorityq)

# # create auto tiler lib
# set(autotiler autotiler)
# set(autotilersrcs autoTiler.cpp)
# add_library("${autotiler}" STATIC ${autotilersrcs})
# set_target_properties(autotiler PROPERTIES LINKER_LANGUAGE CXX)
# target_compile_options(autotiler PRIVATE -lopenmp)
# target_include_directories("${autotiler}" PUBLIC .)
# target_link_libraries(${autotiler} ${OpenCV_LIBS})

# # create main executable
# add_executable(disTiler main.cpp)
# target_compile_options(disTiler PRIVATE -std=c++11)
# target_link_libraries(disTiler ${OpenCV_LIBS})
# target_link_libraries(disTiler ${MPI_CXX_LIBRARIES})
# target_link_libraries(disTiler Halide)
# target_link_libraries(disTiler blurjit)
# target_link_libraries(disTiler distiler)
# target_link_libraries(disTiler autotiler)

# #Copy the test images to the build folder
# if(CPY_BASE_SEG_IMG)
#     add_custom_target(cpyImagesDTH 
#         COMMAND ${CMAKE_COMMAND} -E copy 
#         ${Segmentation_SOURCE_DIR}/images/* ${CMAKE_CURRENT_BINARY_DIR})
#     add_dependencies(disTiler cpyImagesDTH)

#     add_custom_target(cpyRunDTH 
#         COMMAND ${CMAKE_COMMAND} -E copy 
#         ${CMAKE_CURRENT_SOURCE_DIR}/run.sh ${CMAKE_CURRENT_BINARY_DIR})
#     add_dependencies(disTiler cpyRunDTH)
# endif(CPY_BASE_SEG_IMG)
