#ifndef QUAD_TREE_CUTTING_H_
#define QUAD_TREE_CUTTING_H_

#include <list>
#include <set>
#include <iostream>

#include <opencv/cv.hpp>

#include "tilingUtil.h"
#include "CostFunction.h"

void heightBalancedTrieQuadTreeCutting(const cv::Mat& img, 
    std::list<rect_t>& dense, int nTiles);

void costBalancedQuadTreeCutting(const cv::Mat& img, 
    std::list<rect_t>& dense, int nTiles, TilerAlg_t type, CostFunction* cfunc);

#endif // QUAD_TREE_CUTTING_H_
