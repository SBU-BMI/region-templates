#ifndef LIST_CUTTING_H_
#define LIST_CUTTING_H_

#include <list>
#include <set>
#include <iostream>

#include <opencv/cv.hpp>

#include "tilingUtil.h"

void listCutting(const cv::Mat& img, std::list<rect_t>& dense, 
    int nTiles, TilerAlg_t type);

#endif // LIST_CUTTING_H_
