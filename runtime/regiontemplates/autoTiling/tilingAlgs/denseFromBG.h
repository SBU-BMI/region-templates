#ifndef DENSE_FROM_BG_H_
#define DENSE_FROM_BG_H_

#include <list>
#include <set>
#include <iostream>

#include <opencv/cv.hpp>

#include "tilingUtil.h"

/*****************************************************************************/
/**                           Custom Comparators                            **/
/*****************************************************************************/

struct rect_tCompX{
    bool operator()(const rect_t& a, const rect_t& b) {
        return a.xi < b.xi || (a.xi == b.xi && 
            (a.xo != b.xo || a.yi != b.yi || a.yo != b.yo));
    }
};

struct rect_tCompY{
    bool operator()(const rect_t& a, const rect_t& b) {
        return a.yo < b.yo || (a.yo == b.yo && 
            (a.xo != b.xo || a.yi != b.yi || a.xi != b.xi));
    }
};

/*****************************************************************************/
/**                               Main Tiler                                **/
/*****************************************************************************/

void tileDenseFromBG(cv::Mat& mask, std::list<rect_t>& dense, 
    std::list<rect_t>& output, const cv::Mat* input = NULL);

#endif // DENSE_FROM_BG_H_
