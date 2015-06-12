cmake_minimum_required(VERSION 2.6)
project(SystemTests)

# Make sure the compiler can find the includes of the runtime system 
include_directories(${Runtime_SOURCE_DIR})
include_directories(${Runtime_SOURCE_DIR}/ResourceManager)
include_directories(${Runtime_SOURCE_DIR}/util/)
include_directories(${Runtime_SOURCE_DIR}/regiontemplates/)
include_directories(${Runtime_SOURCE_DIR}/regiontemplates/cache/)

# Make sure the compiler can find the library in runtime system directory
link_directories(${Runtime_SOURCE_DIR})

# Create executables to be built
add_executable(TestArgumentsClass TestArguments.cpp)
add_executable(TestComponentsClass TestComponents.cpp)


# Link executables to the runtime system library (just to have access to classes they test)
target_link_libraries(TestArgumentsClass runtimesystem)
target_link_libraries(TestComponentsClass runtimesystem)