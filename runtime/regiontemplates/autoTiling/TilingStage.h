#ifndef TILING_STAGE_H_
#define TILING_STAGE_H_

// #include <string>
// #include <list>
// #include <vector>

#include <opencv/cv.hpp>

// #include "openslide.h"

// #include "TiledRTCollection.h"
// #include "costFuncs/BGMasker.h"

#include "TilingStage.h"

class TilingStage {
public:
    TilingStage();

    // Tiling of unfinishedTiles which updates it and adds tiles to finishedTiles
    virtual void tile(const cv::Mat& refImg, 
        std::vector<cv::Rect_<int64_t>>& unfinishedTiles,
        std::vector<cv::Rect_<int64_t>>& finishedTiles);

    virtual std::string getName() = 0;
};

#endif // TILING_STAGE_H_
