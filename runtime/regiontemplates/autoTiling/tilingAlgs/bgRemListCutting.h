#ifndef BG_REM_LIST_CUTTING_H_
#define BG_REM_LIST_CUTTING_H_

#include <list>

#include <opencv/cv.hpp>

#include "tilingUtil.h"

void bgRemListCutting(const cv::Mat &img, std::list<rect_t> &dense, int nTiles,
                      CostFunction *cfunc, std::list<rect_t> &bgPartitions);

#endif // LIST_CUTTING_H_
