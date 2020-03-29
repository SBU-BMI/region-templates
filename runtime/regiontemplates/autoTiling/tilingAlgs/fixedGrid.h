#ifndef FIXED_GRID_H_
#define FIXED_GRID_H_

#include <iostream>
#include <cmath>
#include <list>

#include <opencv/cv.hpp>

int fixedGrid(int64_t nTiles, int64_t w, int64_t h, int64_t mw, int64_t mh, 
        std::list<cv::Rect_<int64_t>>& rois);

#endif // FIXED_GRID_H_
