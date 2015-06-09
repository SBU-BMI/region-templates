cmake_minimum_required(VERSION 2.6)
project(ExamplesSys)

add_subdirectory("HelloWorld")
add_subdirectory("PipelineTasks")

set(BUILD_NUCLEAR_PIPELINE OFF CACHE BOOL "Include compilation of the nuclear
analysis pipeline")
if(BUILD_NUCLEAR_PIPELINE)
	add_subdirectory("NucleiPipeline-Bundle")
endif(BUILD_NUCLEAR_PIPELINE)
#add_subdirectory("MorphRecon")
#add_subdirectory("DistTransf")
#add_subdirectory("DistTransf-Asyn")
