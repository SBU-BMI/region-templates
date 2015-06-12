cmake_minimum_required(VERSION 2.6)
project(Runtime)

# The version number
set(ExecEngine_VERSION_MAJOR 1)
set(ExecEngine_VERSION_MINOR 0)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

#adding -DDEBUG flag used in the project for debugging purpose
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")


#data spaces path

find_package(MPI REQUIRED)
find_package(OpenCV REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

set(USE_CUDA OFF CACHE BOOL "Include NVidia CUDA Runtime support")
if(USE_CUDA)
    find_package(CUDA 4.0)
    
    if(CUDA_FOUND) 		
        message(STATUS "CUDA detected: " ${CUDA_VERSION})

        # These vars will be processed in other scripts
     	set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} --use_fast_math -arch=sm_20 -DPREFIX_SUM)

        message(STATUS "CUDA NVCC target flags: ${CUDA_NVCC_FLAGS}")
	add_definitions( -DWITH_CUDA )
    endif(CUDA_FOUND)
else(USE_CUDA)
	set(CUDA_FOUND 0)
endif(USE_CUDA)

# Source files used by the Runtime System
set( libsyssrcs SysEnv.cpp Worker.cpp Manager.cpp Argument.cpp PipelineComponentBase.cpp CallBackComponentExecution.cpp ./util/Util.cpp ./util/DataPack.cpp )

set( libname runtimesystem )

# if build same applications is set, than build them: currently just one
add_subdirectory("ResourceManager")

set(USE_REGION_TEMPLATES OFF CACHE BOOL "Include compilation of the Region Templates framework")

if(USE_REGION_TEMPLATES)
	# add region template compiling flag
	add_definitions( -DWITH_RT )

	# choose whether data spaces will be used or not 
	set(USE_DATA_SPACES OFF CACHE BOOL "Include support for Data Spaces in the Region Templates framework")

	if(USE_DATA_SPACES)
		add_definitions( -DWITH_DATA_SPACES )
	endif(USE_DATA_SPACES)
	
	set(USE_ACTIVE_HARMONY OFF CACHE BOOL "Include compilation of Activer Harmony")

	
	
	# data spaces library locations
	link_directories(${RegionTemplate_SOURCE_DIR}/external-src/dataspaces-ib-1.03beta/)

	include_directories( ${RegionTemplate_SOURCE_DIR}/external-src/dataspaces-ib-1.03beta/ )
	include_directories( ${RegionTemplate_SOURCE_DIR}/cache/)
	# region template for compilation
	add_subdirectory("regiontemplates")
endif(USE_REGION_TEMPLATES)	

add_subdirectory("tests")
add_subdirectory("examples")


include_directories(./)
include_directories(./util)
include_directories(./ResourceManager)
include_directories(./regiontemplates)
include_directories(./regiontemplates/Utils)

# Add rule to generate the Runtime System, as a shared library (.so)
add_library("${libname}" SHARED ${libsyssrcs})

if(USE_ACTIVE_HARMONY)
	add_custom_target(
	active_harmony_lib
	COMMAND make install PREFIX=${RegionTemplate_BINARY_DIR}/external-src/activeharmony-4.5/
	WORKING_DIRECTORY ${RegionTemplate_SOURCE_DIR}/external-src/activeharmony-4.5/)
	add_dependencies("${libname}" active_harmony_lib)
endif(USE_ACTIVE_HARMONY)
	
#set_target_properties("${libname}" PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}" -DPROFILE) 
target_link_libraries("${libname}" ${MPI_LIBRARIES} execengine)

if(USE_DATA_SPACES)
       target_link_libraries("${libname}" -ldspacesAll -lrdmacm)
endif(USE_DATA_SPACES)

if(USE_REGION_TEMPLATES)
	target_link_libraries("${libname}" regiontemplates)
endif(USE_REGION_TEMPLATES)
