#ifndef THRESHOLD_TILING_STAGE_H_
#define THRESHOLD_TILING_STAGE_H_

// #include <string>
// #include <list>
// #include <vector>

#include <opencv/cv.hpp>

// #include "openslide.h"

// #include "TiledRTCollection.h"
// #include "costFuncs/BGMasker.h"

#include "TilingStage.h"

class ThresholdBGRemStage : public TilingStage {
public:
    ThresholdBGRemStage();

    // Tiling of unfinishedTiles which updates it and adds tiles to finishedTiles
    void tile(const cv::Mat& refImg, 
        std::vector<cv::Rect_<int64_t>>& unfinishedTiles,
        std::vector<cv::Rect_<int64_t>>& finishedTiles) override;

    std::string getName() override;

    // Parameters
    void setThrhd(std::string arg);
    void setOrient(std::string arg);
    void setErode(std::string arg);
};

#endif // THRESHOLD_TILING_STAGE_H_
