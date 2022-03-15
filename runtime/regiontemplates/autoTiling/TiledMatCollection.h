#ifndef TILED_MAT_COLLECTION_H_
#define TILED_MAT_COLLECTION_H_

#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <vector>

#include <opencv/cv.hpp>

#include "CostFunction.h"
#include "RegionTemplate.h"
#include "svs/svsUtils.h"
#include "tilingAlgs/tilingUtil.h"

class TiledMatCollection {
  public:
    TiledMatCollection(){};

    virtual void tileMat(cv::Mat &mat, std::list<cv::Rect_<int64_t>> &tiles,
                         std::list<cv::Rect_<int64_t>> &bgTiles) = 0;
    std::list<cv::Rect_<int64_t>> tileMat(cv::Mat &mat) {
        // Adds single tile with full image
        std::list<cv::Rect_<int64_t>> tiles;
        cv::Rect_<int64_t>            roi(0, 0, mat.cols, mat.rows);
        tiles.push_back(roi);
        std::list<cv::Rect_<int64_t>> tmp;
        this->tileMat(mat, tiles, tmp);
        return tiles;
    };
};

#endif
