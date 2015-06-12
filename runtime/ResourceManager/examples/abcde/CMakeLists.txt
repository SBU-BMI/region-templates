cmake_minimum_required(VERSION 2.6)
project(abcde)

# Make sure the compiler can find the includes in the Execution Engine directory
include_directories(${ExecEngine_SOURCE_DIR})

# Make sure the compiler can find the library in Execution Engine directory
link_directories(${ExecEngine_SOURCE_DIR})

# Add an executable to be built from the files listed
add_executable(abcde TaskSum.cpp TaskMultiply.cpp Main.cpp)

# Link the executable to the execution engine library
target_link_libraries(abcde execengine)
