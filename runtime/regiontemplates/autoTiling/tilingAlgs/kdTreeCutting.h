#ifndef KD_TREE_CUTTING_H_
#define KD_TREE_CUTTING_H_

#include <list>
#include <iostream>

#include <opencv/cv.hpp>

#include "tilingUtil.h"

void kdTreeCutting(const cv::Mat& img, std::list<rect_t>& dense, 
    int nTiles, TilerAlg_t type);

#endif // KD_TREE_CUTTING_H_