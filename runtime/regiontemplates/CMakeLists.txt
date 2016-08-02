cmake_minimum_required(VERSION 2.6)
project(RegionTemplate)

add_definitions( -ggdb -gdwarf-2 )

# The version number
set(RegionTemplates_VERSION_MAJOR 1)
set(RegionTemplates_VERSION_MINOR 0)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})


#### OpenCV
find_package( OpenCV REQUIRED )

# Source files used by the Runtime System
set( librtemplates BasicDataSource.cpp BoundingBox.cpp DataRegion.cpp ParameterSet.cpp DataRegionFactory.cpp DenseDataRegion2D.cpp 
	RegionTemplate.cpp ./Utils/FileUtils.cpp RTPipelineComponentBase.cpp RegionTemplateCollection.cpp TaskIO.cpp 
	RTCInterface.cpp ./cache/Cache.cpp ./cache/CacheComponent.cpp ./cache/CachePolicy.cpp ./cache/CachePolicyLRU.cpp ./cache/TaskMoveDR2Global.cpp)

set( libname regiontemplates )

include_directories(./Utils/)
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util/)
include_directories(${Runtime_SOURCE_DIR}/regiontemplates/)
include_directories(${Runtime_SOURCE_DIR}/regiontemplates/cache/)

include_directories(${RegionTemplate_SOURCE_DIR}/external-src/dataspaces-ib-1.03beta/)



#data spaces path
#link_directories(${RegionTemplate_SOURCE_DIR}/external-src/dataspaces-ib-1.03beta/)

# Add rule to generate the RegionTemplates, as a shared library 
add_library("${libname}" SHARED ${librtemplates})

#set_target_properties("${libname}" PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}") 
target_link_libraries("${libname}" ${MPI_LIBRARIES} ${OpenCV_LIBS})

#if(USE_DATA_SPACES)
#	target_link_libraries("${libname}" -ldspacesAll -lrdmacm)
#endif(USE_DATA_SPACES)


#target_link_libraries("${libname}" runtimesystem)

option (RTEMPLATES_EXAMPLES "Enable compilation of Region Templates examples" OFF)

if(RTEMPLATES_EXAMPLES)
	add_subdirectory("examples")
endif(RTEMPLATES_EXAMPLES)


option (RTEMPLATES_UNIT_TESTS "Enable compilation of Region Templates unit tests" OFF)

################################
# GTest for Region Templates
################################
if(RTEMPLATES_UNIT_TESTS)
	ADD_SUBDIRECTORY (external-src/gtest-1.6.0)
	enable_testing()
	include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
	add_subdirectory("tests")
endif(RTEMPLATES_UNIT_TESTS)
