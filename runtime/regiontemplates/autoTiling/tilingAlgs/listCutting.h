#ifndef LIST_CUTTING_H_
#define LIST_CUTTING_H_

#include <iostream>
#include <list>
#include <set>

#include <opencv/cv.hpp>

#include "CostFunction.h"
#include "tilingUtil.h"

void listCutting(const cv::Mat& img, std::list<rect_t>& dense, int nTiles,
                 TilerAlg_t type, CostFunction* cfunc);
int listCutting(const cv::Mat& img, std::list<rect_t>& dense, int cpuCount,
                int gpuCount, float cpuPats, float gpuPats,
                CostFunction* cfunc);

#endif  // LIST_CUTTING_H_
