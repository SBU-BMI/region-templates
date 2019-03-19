#ifndef AUTO_TILER_H
#define AUTO_TILER_H

#define CV_MAX_PIX_VAL 255
#define CV_THR_BIN 0
#define CV_THR_BIN_INV 1

#include <list>
#include <algorithm>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv/cv.hpp>

#include <omp.h>

#include "distTiling.h"

std::list<rect_t> autoTiler(cv::Mat& input, int border=10, 
    int bgThreshold=50, int erosionSize=60); // erosion 20 ok

#endif
