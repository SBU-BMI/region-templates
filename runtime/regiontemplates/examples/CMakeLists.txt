cmake_minimum_required(VERSION 2.6)
project(ExamplesRT)

#add_subdirectory("PipelineRTFS-SF")
#add_subdirectory("PipelineRTFS-NSF")

#Difference analysis basic pipelines
add_subdirectory("PipelineRTFS-NS-Diff")
if (USE_ACTIVE_HARMONY)
add_subdirectory("PipelineRTFS-NS-Diff-AH")
#add_subdirectory("PipelineRTFS-NS-Diff-AH-PRO")
if (USE_YI)
#add_subdirectory("PipelineRTFS-NS-Diff-AH-PRO-Yi")
endif ()
endif ()

#add_subdirectory("PipelineRTFS-NSF-Diff")
#add_subdirectory("HelloWorldRT")
#add_subdirectory("RTPipelineBundle")
#add_subdirectory("FireDetection")
