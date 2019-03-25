cmake_minimum_required(VERSION 2.6)
project(ExamplesRT)

#add_subdirectory("PipelineRTFS-NSF")

#Difference analysis basic pipelines
option(USE_FINE_GRAIN_SEG_TASKS "Enable use of fine grain segmentation tasks of NScale for tuning optimization" OFF)
option(USE_HADOOPGIS "Compile HadoopGIS examples (Requires Hadoopgis)" OFF)

if (RT_TUNING_NSCALE_EXAMPLE)
    option(RT_TUNING_NSCALE_EXAMPLE "Enable compilation of NScale example with tuning" OFF)
    option(DISTRIBUTED_TILLING_EXAMPLE "Enable compilation of a disTiler tool example" OFF)
    add_subdirectory("PipelineRTFS-NS-Diff")
    if (USE_ACTIVE_HARMONY)
        add_subdirectory("PipelineRTFS-SF")
        add_subdirectory("PipelineRTFS-NS-Diff-AH")
        add_subdirectory("PipelineRTFS-NS-Diff-AH-PRO")
    endif (USE_ACTIVE_HARMONY)
    if(DISTRIBUTED_TILLING_EXAMPLE)
        add_subdirectory("PipelineRTFS-NS-Diff-disTiler")
    endif(DISTRIBUTED_TILLING_EXAMPLE)
    if (USE_HADOOPGIS)
        find_path(HADOOPGIS_BUILD_DIR REQUIRED)
        set(HADOOPGIS_SCRIPTS_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}) #the script path is the same as the executables path
        set(WRITE_ENABLED_TEMP_PATH /tmp/ CACHE PATH "Path to a temp directory with write permission")
        #set(HADOOPGIS_BUILD_DIR CACHE PATH "Hadoopgis Build Directory")


        if (NOT IS_DIRECTORY ${WRITE_ENABLED_TEMP_PATH})
            message(SEND_ERROR "Temp folder: ${WRITE_ENABLED_TEMP_PATH} IS NOT A VALID DIRECTORY")
        endif ()

        if (NOT IS_DIRECTORY ${HADOOPGIS_BUILD_DIR})
            message(SEND_ERROR "HADOOPGIS_BUILD_DIR:${HADOOPGIS_BUILD_DIR} IS NOT A VALID DIRECTORY")
        endif ()

        if ((NOT EXISTS ${HADOOPGIS_BUILD_DIR}/bin/resque) OR (NOT EXISTS ${HADOOPGIS_BUILD_DIR}/bin/map_obj_to_tile) OR (NOT EXISTS ${HADOOPGIS_BUILD_DIR}/bin/mbb_normalizer))
            message(SEND_ERROR "HADOOPGIS_BUILD_DIR:${HADOOPGIS_BUILD_DIR} The directory that you inserted is not a valid build directory.")
        endif ()

        add_subdirectory("PipelineRTFS-NS-Diff-GIS")
        add_subdirectory("PipelineRTFS-NS-KNN")
        add_subdirectory("PipelineRTFS-NS-KNN-Bounded")
        if (USE_ACTIVE_HARMONY)
        add_subdirectory("PipelineRTFS-NS-Diff-AH-GIS")
        add_subdirectory("Tuning-Nscale")
        add_subdirectory("CrossValidation-Nscale")
        add_subdirectory("PipelineRTFS-NS-Diff-AH-PRO-GIS")
        endif (USE_ACTIVE_HARMONY)

    endif (USE_HADOOPGIS)
    if (USE_FINE_GRAIN_SEG_TASKS)
        add_subdirectory("PipelineRTFS-NS-Diff-FGO")
    endif ()
endif (RT_TUNING_NSCALE_EXAMPLE)

option(RT_TUNING_YI_EXAMPLE "Enable compilation of Yi's examples with tuning" OFF)

if (RT_TUNING_YI_EXAMPLE)
    if (USE_ACTIVE_HARMONY)
        add_subdirectory("PipelineRTFS-NS-Diff-AH-PRO-Yi")
        if (USE_HADOOPGIS)
            add_subdirectory("PipelineRTFS-NS-Diff-AH-PRO-Yi-GIS")
            add_subdirectory("Tuning-Yi")
            add_subdirectory("CrossValidation-Yi")
        endif (USE_HADOOPGIS)
    endif (USE_ACTIVE_HARMONY)
endif (RT_TUNING_YI_EXAMPLE)

option (RT_PARAMETER_STUDY_NSCALE_EXAMPLE "Enable compilation of NScale example with Parameter Study" OFF)

if(RT_PARAMETER_STUDY_NSCALE_EXAMPLE)
	add_subdirectory("PipelineRTFS-NS-ParStudy")
endif(RT_PARAMETER_STUDY_NSCALE_EXAMPLE)

option (RT_PARAMETER_STUDY_YI_EXAMPLE "Enable compilation of Yi's example with Parameter Study" OFF)
if(RT_PARAMETER_STUDY_YI_EXAMPLE)                                      
        add_subdirectory(Pipeline-Yi-ParStudy)                         
endif(RT_PARAMETER_STUDY_YI_EXAMPLE)   

option (RT_YI_EXAMPLE "Enable compilation of Yi's Pipeline" OFF)
if(RT_YI_EXAMPLE)
	add_subdirectory(PipelineRTFS-SF-Yi)
endif(RT_YI_EXAMPLE)

add_subdirectory("HelloWorldRT")
#add_subdirectory("PipelineRTFS-NSF-Diff")
#add_subdirectory("HelloWorldRT")
#add_subdirectory("RTPipelineBundle")
#add_subdirectory("FireDetection")
