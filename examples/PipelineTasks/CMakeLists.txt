cmake_minimum_required(VERSION 2.6)
project(PipelineTasks)

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util/)
include_directories(${Runtime_SOURCE_DIR}/regiontemplates/)
include_directories(${Runtime_SOURCE_DIR}/regiontemplates/cache/)

# Make sure the compiler can find the library in runtime system directory
link_directories(${Runtime_SOURCE_DIR})

# Create library with components implementation
set(libname componentsPipeTasks)

# Source files used by the Runtime System
set( libcomponentssrcs PipelineComponent.cpp TaskCompute.cpp TaskRead.cpp FileUtils.cpp)

# Add rule to generate the Runtime System, as a shared library (.so)
add_library("${libname}" SHARED ${libcomponentssrcs})

#set_target_properties("${libname}" PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}") 
target_link_libraries("${libname}" runtimesystem)

# Create executable to be built from the files listed
add_executable(PipelineManager PipelineManager.cpp)

# Link the executable to the runtime system library
target_link_libraries(PipelineManager runtimesystem "${libname}")