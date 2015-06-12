SHELL   = /bin/sh
CC=mpicc -fPIC -fpermissive
CPP=mpic++ -fPIC
CFLAGS=-g -c
FTN=mpif90
LDFLAGS=-lrdmacm

OPENCV_ROOT=/lustre/medusa/gteodor/opencv/OpenCV-2.3.1
OPENCV_BUILD_ROOT=/lustre/medusa/gteodor/opencv/OpenCV-2.3.1-build
OPENCV_ROOT=/lustre/medusa/gteodor/OpenCV-2.3.1/
OPENCV_BUILD_ROOT=${OPENCV_ROOT}

OPENCV_INCLUDE_DIRS=-I../ -I${OPENCV_ROOT}/include/ -I${OPENCV_ROOT}/modules/core/include/ -I${OPENCV_ROOT}/modules/flann/include/ -I${OPENCV_ROOT}/modules/imgproc/include/ -I${OPENCV_ROOT}/modules/video/include/ -I${OPENCV_ROOT}/modules/features2d/include/ -I${OPENCV_ROOT}/modules/objdetect/include/ -I${OPENCV_ROOT}/modules/calib3d/include/ -I${OPENCV_ROOT}/modules/ml/include/ -I${OPENCV_ROOT}/modules/highgui/include/ $(LDFLAGS) -I${OPENCV_ROOT}/modules/contrib/include/  -I${OPENCV_ROOT}/modules/gpu/include/


test_put_images_2D: ../util.o ReadInputFileNames.o Util.o FileUtils.o test_put_images_2D.c
	$(CPP) -o $@ test_put_images_2D.c Util.o ReadInputFileNames.o FileUtils.o -L.. -ldspacesAll ${OPENCV_INCLUDE_DIRS} -L${OPENCV_BUILD_ROOT}/lib  -lopencv_highgui -lopencv_core


test_get_images_2D: ../util.o ReadInputFileNames.o Util.o FileUtils.o test_get_images_2D.c
	$(CPP) -o $@ test_get_images_2D.c Util.o ReadInputFileNames.o FileUtils.o -L.. -ldspacesAll ${OPENCV_INCLUDE_DIRS} -L${OPENCV_BUILD_ROOT}/lib  -lopencv_highgui -lopencv_core

convert2Raw: convert2Raw.c
	$(CPP) -o $@ convert2Raw.c ReadInputFileNames.o FileUtils.o ${OPENCV_INCLUDE_DIRS} -L${OPENCV_BUILD_ROOT}/lib  -lopencv_highgui -lopencv_core


test: test_put_images_2D  test_get_images_2D convert2Raw


default: all

lib: $(OUT1) $(OUT2) $(OUT3)

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@
.f90.o:
	$(FTN) -g -c $< -o $@
.cpp.o:
	$(CPP) $(INCLUDES) $(CFLAGS) -c $< -o $@


restart:
	rm -f conf srv.lck 

clean:
	rm -f test_put_images_2D conf srv.lck *.o


    
