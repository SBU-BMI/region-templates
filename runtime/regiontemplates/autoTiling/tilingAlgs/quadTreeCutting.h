#ifndef QUAD_TREE_CUTTING_H_
#define QUAD_TREE_CUTTING_H_

#include <list>
#include <iostream>

#include <opencv/cv.hpp>

#include "tilingUtil.h"

void trieQuadTreeCutting(const cv::Mat& img, 
    std::list<rect_t>& dense, int nTiles);

#endif // QUAD_TREE_CUTTING_H_
