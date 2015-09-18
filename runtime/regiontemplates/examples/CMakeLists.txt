cmake_minimum_required(VERSION 2.6)
project(ExamplesRT)

add_subdirectory("PipelineRTFS-SF")
#add_subdirectory("PipelineRTFS-NSF")

#Difference analysis basic pipelines
#add_subdirectory("PipelineRTFS-NS-Diff")
#add_subdirectory("PipelineRTFS-NS-Diff-AH")
add_subdirectory("PipelineRTFS-NS-Diff-AH-PRO")
add_subdirectory("PipelineRTFS-NS-Diff-AH-PRO-Yi")

#add_subdirectory("PipelineRTFS-NSF-Diff")
#add_subdirectory("HelloWorldRT")
#add_subdirectory("RTPipelineBundle")
#add_subdirectory("FireDetection")
