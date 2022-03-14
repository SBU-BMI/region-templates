#ifndef FINE_BG_REM_H_
#define FINE_BG_REM_H_

#include "tilingAlgs/tilingUtil.h"
#include <list>
#include <opencv/cv.hpp>

std::list<rect_t> fineBgRemoval(cv::Mat img, std::list<rect_t> tiles);

#endif // FINE_BG_REM_H_