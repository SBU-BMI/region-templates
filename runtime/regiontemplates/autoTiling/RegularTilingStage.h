#ifndef REGULAR_TILING_STAGE_H_
#define REGULAR_TILING_STAGE_H_

#include <string>
// #include <list>
// #include <vector>

#include <opencv/cv.hpp>

// #include "openslide.h"

// #include "TiledRTCollection.h"
// #include "costFuncs/BGMasker.h"

#include "TilingStage.h"

class RegularTilingStage : public TilingStage {
public:
    RegularTilingStage();

    // Tiling of unfinishedTiles which updates it and adds tiles to finishedTiles
    void tile(const cv::Mat& refImg, 
        std::vector<cv::Rect_<int64_t>>& unfinishedTiles,
        std::vector<cv::Rect_<int64_t>>& finishedTiles) override;

    std::string getName() override;

    // Parameters
    void setTileSize(std::string arg);
};

#endif // REGULAR_TILING_STAGE_H_
