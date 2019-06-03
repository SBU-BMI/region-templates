#ifndef REGULAR_TILING_STAGE_H_
#define REGULAR_TILING_STAGE_H_

#include <string>
// #include <list>
// #include <vector>

#include <opencv/cv.hpp>

#include "TilingStage.h"

class RegularTilingStage : public TilingStage {
public:
    RegularTilingStage();

    std::string getName() override;

    // Parameters
    void setTileSize(std::string arg);

    // Tiling of unfinishedTiles which updates it and adds tiles to finishedTiles
    void tile(const cv::Mat& refImg, 
        std::vector<cv::Rect_<int64_t>>& unfinishedTiles,
        std::vector<cv::Rect_<int64_t>>& finishedTiles) override;
};

#endif // REGULAR_TILING_STAGE_H_
