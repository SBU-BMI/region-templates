cmake_minimum_required(VERSION 2.6)
project(Tuning)

find_package(OpenCV REQUIRED)


find_package(ITK)
if (ITK_FOUND)
    include(${ITK_USE_FILE})
endif (ITK_FOUND)

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util)
include_directories(${RegionTemplate_SOURCE_DIR})
include_directories(${RegionTemplate_SOURCE_DIR}/Utils)

# nscale dirs
find_path(YI_SRC_DIR REQUIRED)
find_path(YI_BUILD_DIR REQUIRED)
set(yisrcdir ${YI_SRC_DIR})
set(yibuilddir ${YI_BUILD_DIR})

#includes
include_directories(${yisrcdir}/)


if (USE_ACTIVE_HARMONY)
    include_directories(${RegionTemplate_BINARY_DIR}/external-src/activeharmony-4.5/include/)
endif (USE_ACTIVE_HARMONY)

# Make sure the compiler can find the library in runtime system directory
link_directories(${Runtime_SOURCE_DIR})
# nscale lib dir
#link_directories(/Users/george/Documents/emory/nscale-sc13/nscale/lib)

link_directories(${yibuilddir}/)

# Create library with components implementation
set(libname componenttuning)

# Source files used by the Runtime System
set(libcomponentssrcs Segmentation.cpp DiceMaskComp.cpp DiceNotCoolMaskComp.cpp TaskSegmentation.cpp)

#Hadoopgis Dependencies

set(libcomparativeanalysis ../../comparativeanalysis/TaskDiffMask.cpp
        ../../comparativeanalysis/pixelcompare/PixelCompare.cpp ../../comparativeanalysis/hadoopgis/Hadoopgis.cpp
        ../../comparativeanalysis/hadoopgis/predicates/DiceCoefficient.cpp
        ../../comparativeanalysis/hadoopgis/predicates/JaccardIndex.cpp
        ../../comparativeanalysis/hadoopgis/predicates/MaskIntersection.cpp
        ../../comparativeanalysis/hadoopgis/predicates/DiceNotCoolCoefficient.cpp)
#Replace Macro inside program
add_definitions(-DHADOOPGIS_SCRIPTS_DIRECTORY=${HADOOPGIS_SCRIPTS_OUTPUT_DIR})
add_definitions(-DWRITE_ENABLED_TEMPORARY_PATH=${WRITE_ENABLED_TEMP_PATH})
add_definitions(-DHADOOPGIS_BUILD_DIRECTORY=${HADOOPGIS_BUILD_DIR})


#Copy the hadoopgis scripts to the executable build folder everytime the program is compiled.
add_custom_target(copyScriptsToTuning COMMAND ${CMAKE_COMMAND} -E copy_directory ${RegionTemplate_SOURCE_DIR}/comparativeanalysis/hadoopgis/scripts/ ${HADOOPGIS_SCRIPTS_OUTPUT_DIR})

#End of  Hadoopgis Dependencies


# Add rule to generate the Runtime System, as a shared library (.so)
add_library("${libname}" SHARED ${libcomponentssrcs} ${libcomparativeanalysis})

add_dependencies("${libname}" copyScriptsToTuning)  #End of  Hadoopgis Dependencies

#set_target_properties("${libname}" PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}") 
target_link_libraries("${libname}" runtimesystem)
target_link_libraries(${libname} regiontemplates)
set(CMAKE_CXX_FLAGS "-fopenmp")
set(CMAKE_C_FLAGS "-fopenmp")

target_link_libraries(${libname} ${YI_BUILD_DIR}/liblocalLibraries.a)


target_link_libraries("${libname}" ${OpenCV_LIBS} ${ITK_LIBRARIES})

#Copy the cache-conf.xml file located in the example folder to the build folder everytime the program is compiled.
add_custom_target(copyCacheConfToTuning COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/cache-conf/rtconf.xml ${CMAKE_CURRENT_BINARY_DIR})

# Create executable to be built from the files listed
add_executable(Tuning PipelineManager.cpp)
if (USE_ACTIVE_HARMONY)
    target_link_libraries(Tuning ${RegionTemplate_BINARY_DIR}/external-src/activeharmony-4.5/lib/libharmony.a)
endif (USE_ACTIVE_HARMONY)
add_dependencies(Tuning copyCacheConfToTuning)

# Link the executable to the runtime system library
target_link_libraries(Tuning runtimesystem "${libname}")
target_link_libraries(Tuning regiontemplates)
