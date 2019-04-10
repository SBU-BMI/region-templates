cmake_minimum_required(VERSION 2.6)
project(RegionTemplate)

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
set(librtemplates BasicDataSource.cpp BoundingBox.cpp DataRegion.cpp ParameterSet.cpp DataRegionFactory.cpp DenseDataRegion2D.cpp DataRegion2DUnaligned.cpp
	RegionTemplate.cpp ./Utils/FileUtils.cpp RTPipelineComponentBase.cpp RegionTemplateCollection.cpp TaskIO.cpp
	RTCInterface.cpp ./cache/Cache.cpp ./cache/CacheComponent.cpp ./cache/CachePolicy.cpp ./cache/CachePolicyLRU.cpp ./cache/TaskMoveDR2Global.cpp
	./autotuning/geneticalgorithm/GeneticAlgorithm.cpp ./autotuning/geneticalgorithm/Initialization/RandomInitialization.cpp
	./autotuning/geneticalgorithm/Mating/CutSwapMating.cpp ./autotuning/geneticalgorithm/Mutation/RandomMutation.cpp ./autotuning/geneticalgorithm/Propagation/EliteMembersPropagation.cpp
	./autotuning/geneticalgorithm/Selection/NaturalSelection.cpp ./autotuning/geneticalgorithm/Selection/ParentOffspringSelection.cpp
	./autotuning/geneticalgorithm/Sorting/InsertionSorting.cpp ./autotuning/geneticalgorithm/Utilities/GAUtilities.cpp)
	
# Source files used by the Runtime System with ActiveHarmony
if(USE_ACTIVE_HARMONY)
	find_path(AH_SRC_DIR REQUIRED)
	set(ahsrcdir ${AH_SRC_DIR})
	set(librtemplates BasicDataSource.cpp BoundingBox.cpp DataRegion.cpp ParameterSet.cpp DataRegionFactory.cpp DenseDataRegion2D.cpp DataRegion2DUnaligned.cpp
		RegionTemplate.cpp ./Utils/FileUtils.cpp RTPipelineComponentBase.cpp RegionTemplateCollection.cpp TaskIO.cpp
		RTCInterface.cpp ./cache/Cache.cpp ./cache/CacheComponent.cpp ./cache/CachePolicy.cpp ./cache/CachePolicyLRU.cpp ./cache/TaskMoveDR2Global.cpp
		autotuning/activeharmony/ActiveHarmonyTuning.cpp ./autotuning/geneticalgorithm/GeneticAlgorithm.cpp ./autotuning/geneticalgorithm/Initialization/RandomInitialization.cpp
		./autotuning/geneticalgorithm/Mating/CutSwapMating.cpp ./autotuning/geneticalgorithm/Mutation/RandomMutation.cpp ./autotuning/geneticalgorithm/Propagation/EliteMembersPropagation.cpp
		./autotuning/geneticalgorithm/Selection/NaturalSelection.cpp ./autotuning/geneticalgorithm/Selection/ParentOffspringSelection.cpp
		./autotuning/geneticalgorithm/Sorting/InsertionSorting.cpp ./autotuning/geneticalgorithm/Utilities/GAUtilities.cpp)
endif(USE_ACTIVE_HARMONY)

set( libname regiontemplates )

include_directories(./Utils/)
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util/)
include_directories(${Runtime_SOURCE_DIR}/regiontemplates/)
include_directories(${Runtime_SOURCE_DIR}/regiontemplates/cache/)

include_directories(${RegionTemplate_SOURCE_DIR}/external-src/dataspaces-ib-1.03beta/)

if(USE_ACTIVE_HARMONY)
	find_path(AH_SRC_DIR REQUIRED)
	set(ahsrcdir ${AH_SRC_DIR})
	include_directories(${ahsrcdir}/include/)
endif(USE_ACTIVE_HARMONY)

option (RTEMPLATES_EXAMPLES "Enable compilation of Region Templates examples" OFF)

if(RTEMPLATES_EXAMPLES)
	add_subdirectory("examples")
endif(RTEMPLATES_EXAMPLES)

# add TiledRTCollection and required libs if compiled
if(DISTRIBUTED_TILLING_EXAMPLE)
	set(librtemplates ${librtemplates} TiledRTCollection.cpp RegTiledRTCollection.cpp)
endif(DISTRIBUTED_TILLING_EXAMPLE)

#data spaces path
#link_directories(${RegionTemplate_SOURCE_DIR}/external-src/dataspaces-ib-1.03beta/)

# Add rule to generate the RegionTemplates, as a shared library 
add_library("${libname}" SHARED ${librtemplates})
target_link_libraries("${libname}" ${MPI_LIBRARIES} ${OpenCV_LIBS})

if (USE_ACTIVE_HARMONY)
	find_path(AH_SRC_DIR REQUIRED)
	set(ahsrcdir ${AH_SRC_DIR})
	target_link_libraries("${libname}" ${ahsrcdir}/lib/libharmony.a)
endif (USE_ACTIVE_HARMONY)

# add TiledRTCollection and required libs if compiled
if(DISTRIBUTED_TILLING_EXAMPLE)
	# Find and import OpenSlide
	find_path(OPENSLIDE_SRC_DIR REQUIRED)
	include_directories(${OPENSLIDE_SRC_DIR})
	link_directories(${OPENSLIDE_SRC_DIR}/.libs/)
	set(openslide ${OPENSLIDE_SRC_DIR}/.libs/libopenslide.so)
	target_link_libraries("${libname}" openslide)
endif(DISTRIBUTED_TILLING_EXAMPLE)

#if(USE_DATA_SPACES)
#	target_link_libraries("${libname}" -ldspacesAll -lrdmacm)
#endif(USE_DATA_SPACES)

#target_link_libraries("${libname}" runtimesystem)


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
