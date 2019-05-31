#ifndef LOG_SWEEPING_TILING_STAGE_H_
#define LOG_SWEEPING_TILING_STAGE_H_

// #include <string>
// #include <list>
// #include <vector>

#include <opencv/cv.hpp>

// #include "openslide.h"

// #include "TiledRTCollection.h"
// #include "costFuncs/BGMasker.h"

#include "TilingStage.h"

class LogSweepTilingStage : public TilingStage {
public:
    LogSweepTilingStage();

    // Tiling of unfinishedTiles which updates it and adds tiles to finishedTiles
    void tile(const cv::Mat& refImg, 
        std::vector<cv::Rect_<int64_t>>& unfinishedTiles,
        std::vector<cv::Rect_<int64_t>>& finishedTiles) override;

    std::string getName() override;

    // Parameters
    void setNTiles(const std::string arg);
    void setMaxC(const std::string arg);
    void setVar(const std::string arg);
};

#endif // LOG_SWEEPING_TILING_STAGE_H_
