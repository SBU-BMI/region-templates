cmake_minimum_required(VERSION 2.6)
project(Tests)

# Make sure the compiler can find the includes in the Execution Engine directory
include_directories(${ExecEngine_SOURCE_DIR})

# Make sure the compiler can find the library in Execution Engine directory
link_directories(${ExecEngine_SOURCE_DIR})

# Add executables to be built from the files listed
add_executable(testEngine testEngine.cpp)
add_executable(testQueue testQueue.cpp)
add_executable(testSem testSem.cpp)
add_executable(testTasksDependencyMap testTasksDependencyMap.cpp)
add_executable(testThreadPool testThreadPool.cpp)

# Link the executables to the execution engine library
target_link_libraries(testEngine execengine)
target_link_libraries(testQueue execengine)
target_link_libraries(testSem execengine)
target_link_libraries(testTasksDependencyMap execengine)
target_link_libraries(testThreadPool execengine)