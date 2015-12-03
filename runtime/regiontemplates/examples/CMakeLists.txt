cmake_minimum_required(VERSION 2.6)
project(ExamplesRT)

add_subdirectory("PipelineRTFS-SF")
#add_subdirectory("PipelineRTFS-NSF")


option (RT_TUNING_NSCALE_EXAMPLE "Enable compilation of NScale example with tuning" OFF)

if(RT_TUNING_NSCALE_EXAMPLE)
	#Difference analysis basic pipelines
	#add_subdirectory("PipelineRTFS-NS-Diff")
	#add_subdirectory("PipelineRTFS-NS-Diff-AH")
	add_subdirectory("PipelineRTFS-NS-Diff-AH-PRO")
endif(RT_TUNING_NSCALE_EXAMPLE)


option (RT_TUNING_YI_EXAMPLE "Enable compilation of Yi's examples with tuning" OFF)

if(RT_TUNING_YI_EXAMPLE)
	add_subdirectory("PipelineRTFS-NS-Diff-AH-PRO-Yi")
endif(RT_TUNING_YI_EXAMPLE)

#add_subdirectory("PipelineRTFS-NSF-Diff")
#add_subdirectory("HelloWorldRT")
#add_subdirectory("RTPipelineBundle")
#add_subdirectory("FireDetection")
