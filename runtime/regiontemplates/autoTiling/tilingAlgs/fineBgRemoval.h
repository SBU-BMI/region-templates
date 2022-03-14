#ifndef FINE_BG_REM_H_
#define FINE_BG_REM_H_

#include "tilingAlgs/tilingUtil.h"
#include <list>
#include <opencv/cv.hpp>

void fineBgRemoval(cv::Mat img, std::list<rect_t> initialTiles,
                   std::list<rect_t> &dense, std::list<rect_t> &bg);

#endif // FINE_BG_REM_H_