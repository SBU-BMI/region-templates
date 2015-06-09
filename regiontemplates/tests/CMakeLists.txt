project(RegionTemplateTests)

find_package(OpenMP)
if (OPENMP_FOUND)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Make sure the compiler can find the includes in the Region Templates directory
include_directories(${RegionTemplate_SOURCE_DIR})

# Make sure the compiler can find the library in Region Templates directory
link_directories(${RegionTemplate_SOURCE_DIR})


set(programs Instantiate BoundingBoxTest TestConfigParsing TestCache)
foreach(program ${programs})
        # Add an executable to be built from the files listed
        add_executable(${program} ${program}.cpp)

        # Link the executable to the Region Templates library
        target_link_libraries(${program} regiontemplates gtest gtest_main runtimesystem)

		# 
		add_test(${program} ${program})
endforeach(program)

set(programsSources Instantiate.cpp BoundingBoxTest.cpp)
# Add an executable to be built from the files listed
add_executable(testAll ${programsSources})

# Link the executable to the Region Templates library
target_link_libraries(testAll regiontemplates runtimesystem gtest gtest_main)

# Copy test image files to test folder.
file(COPY 
	${CMAKE_CURRENT_SOURCE_DIR}/data
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)


set(programs TestValgrind  TestDataRegion)

foreach(program ${programs})
		add_executable(${program} ${program}.cpp)
		# Link the executable to the Region Templates library
        target_link_libraries(${program} regiontemplates runtimesystem)
endforeach(program)        
