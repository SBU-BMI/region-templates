cmake_minimum_required(VERSION 2.6)
project(TestPutGetImageC)

#find_package(OpenCV REQUIRED)
#find_package(MPI REQUIRED)

#set(CMAKE_CXX_COMPILER mpic++)
#set(CMAKE_C_COMPILER mpicc)
#
set( CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-fpermissive " )

include_directories(${MPI_INCLUDE_PATH})

# Source files used by the Dart
set( execputgetsrcs ReadInputFileNames.cpp Util.cpp FileUtils.cpp
test_put_images_2D.c )

add_executable( test_put_get_2D ${execputgetsrcs} )

target_link_libraries(test_put_get_2D ${MPI_LIBRARIES} ${OpenCV_LIBS} dart dspaces dscommon )

