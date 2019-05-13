cmake_minimum_required(VERSION 2.6)
project(ExecEngine)

set(libname execengine)

# The version number
set(ExecEngine_VERSION_MAJOR 1)
set(ExecEngine_VERSION_MINOR 1)

# Source files used by Execution Engine
set(libsrcs Task.cpp TasksQueue.cpp ExecutionEngine.cpp ThreadPool.cpp TrackDependencies.cpp TaskArgument.cpp)


# Find threads package which is linked to the execution engine library
find_package( Threads REQUIRED )
find_package( OpenCV REQUIRED )

# Add rule to generate execution engine, as a shared library (.so)
add_library("${libname}" SHARED ${libsrcs})

# add the required libs for svs if compiled
if(WITH_DISTRIBUTED_TILLING_EXAMPLE)
    # Find and import OpenSlide
    find_path(OPENSLIDE_SRC_DIR REQUIRED)
    include_directories(${OPENSLIDE_SRC_DIR})
    link_directories(${OPENSLIDE_SRC_DIR}/.libs/)
    set(openslide ${OPENSLIDE_SRC_DIR}/.libs/libopenslide.so)
    target_link_libraries("${libname}" openslide)
    message(${OPENSLIDE_SRC_DIR})
endif(WITH_DISTRIBUTED_TILLING_EXAMPLE)

# Link Pthreads stuff to the execution engine
target_link_libraries("${libname}" ${CMAKE_THREAD_LIBS_INIT} ${OpenCV_LIBS})

# Check if the user want to build sample applications
option (BUILD_SAMPLE_APPLICATIONS "Inform whether sample applications should be built" ON)

# if build same applications is set, than build them: currently just one
if(BUILD_SAMPLE_APPLICATIONS)
	add_subdirectory("examples/hello/")
	add_subdirectory("examples/taskDependency")
	add_subdirectory("examples/abc")
	add_subdirectory("examples/abcde")
	add_subdirectory("tests")
endif(BUILD_SAMPLE_APPLICATIONS)
