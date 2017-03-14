cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(projName "pipeline20141229")
project(${projName})

set(CMAKE_C_FLAGS "-std=c99")

find_package(OpenMP)
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# ################################################################################
# # ITK, Should be with ItkVtkGlue
# find_package (ITK)
# include(${USE_ITK_FILE})

# ################################################################################
# # put all libs together
# set(Libraries ${Libraries}
#     ${ITK_LIBRARIES}
#     )


################################################################################
# openCV
find_package( OpenCV REQUIRED )
include_directories(${OpenCV_INCLUDE_DIRS})
set(Libraries ${Libraries}
    ${OpenCV_LIBS}
    )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package (GLIB)
if (HAVE_GLIB)
include_directories(${GLIB_INCLUDES})
set(Libraries ${Libraries}
    ${GLIB_LIBRARIES}
    )
endif(HAVE_GLIB)


# ################################################################################
# # find Tiff path
# find_path(Tiff_INSTALLATION_PATH include/tiff.h)
# if(NOT Tiff_INSTALLATION_PATH)
#    message(FATAL_ERROR "Please enter the installation path of Tiff, i.e., the one after --prefix= when configuring Tiff")
# endif(NOT Tiff_INSTALLATION_PATH)

# include_directories(${Tiff_INSTALLATION_PATH}/include/)

# set(TiffLibs
#   ${Tiff_INSTALLATION_PATH}/lib/libtiff.so
#   )

# ################################################################################
# # put all libs together
# set(Libraries ${Libraries}
#     ${TiffLibs}
#     )

################################################################################
# find Jpeg path
# set(Jpeg_INSTALLATION_PATH /home/yigao/usr/package/jpeg-9a-install/)
# include_directories(${Jpeg_INSTALLATION_PATH}/include/)
# set(JpegLibs
#   ${Jpeg_INSTALLATION_PATH}/lib/libjpeg.so
#   )


find_path(Jpeg_INSTALLATION_PATH include/jpeglib.h)
if(NOT Jpeg_INSTALLATION_PATH)
   message(FATAL_ERROR "Please enter the installation path of Jpeg, i.e., the one after --prefix= when configuring Jpeg")
endif(NOT Jpeg_INSTALLATION_PATH)

include_directories(${Jpeg_INSTALLATION_PATH}/include/)

set(JpegLibs
  ${Jpeg_INSTALLATION_PATH}/lib/libjpeg.so
  )

# ################################################################################
# # put all libs together
# set(Libraries ${Libraries}
#     ${JpegLibs}
#     )


################################################################################
# find OpenSlide path
find_path(OpenSlide_INSTALLATION_PATH include/openslide/openslide.h REQUIRED)
if(NOT OpenSlide_INSTALLATION_PATH)
   message(FATAL_ERROR "Please enter the installation path of OpenSlide, i.e., the one after --prefix= when configuring OpenSlide")
endif(NOT OpenSlide_INSTALLATION_PATH)

include_directories(${OpenSlide_INSTALLATION_PATH}/include/openslide/)

set(OpenSlideLibs
  ${OpenSlide_INSTALLATION_PATH}/lib/libopenslide.so
  )


#add_library(openSliceTools openslide-tools-common.c)

add_library(myLib utilitiesSvs.cxx)

################################################################################
# put all libs together
set(Libraries
    myLib
    ${OpenSlideLibs}
    ${Libraries}
    )


################################################################################
# include dir
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_BINARY_DIR}")
#include_directories("${PROJECT_SOURCE_DIR}/lib")
#include_directories("${PROJECT_SOURCE_DIR}/imageProcessing/registration/")
#include_directories(${ALGLIB_PATH})



################################################################################
# include sub-dirs
add_subdirectory(tiler)
#add_subdirectory(test)
