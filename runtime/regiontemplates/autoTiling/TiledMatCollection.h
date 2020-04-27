#ifndef TILED_MAT_COLLECTION_H_
#define TILED_MAT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>
#include <map>
#include <algorithm>

#include <opencv/cv.hpp>

#include "RegionTemplate.h"
#include "svs/svsUtils.h"
#include "tilingAlgs/tilingUtil.h"
#include "CostFunction.h"

class TiledMatCollection {
public:
    TiledMatCollection() {};

    virtual void tileMat(cv::Mat& mat, std::list<cv::Rect_<int64_t>>& tiles) = 0;
    std::list<cv::Rect_<int64_t>> tileMat(cv::Mat& mat) {
        // Adds single tile with full image
        std::list<cv::Rect_<int64_t>> tiles;
        cv::Rect_<int64_t> roi(0, 0, mat.cols, mat.rows);
        tiles.push_back(roi);
        this->tileMat(mat, tiles);
        return tiles;
    };
};

#endif
