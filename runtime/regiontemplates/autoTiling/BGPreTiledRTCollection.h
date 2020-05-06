#ifndef BG_PRE_TILED_RT_COLLECTION_H_
#define BG_PRE_TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>
#include <algorithm>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "TiledRTCollection.h"
#include "TiledMatCollection.h"
#include "costFuncs/BGMasker.h"
#include "CostFunction.h"
#include "tilingAlgs/tilingUtil.h"
#include "tilingAlgs/denseFromBG.h"

class BGPreTiledRTCollection : public TiledRTCollection, public TiledMatCollection {
private:
    BGMasker* bgm;

    std::map<std::string, std::list<cv::Rect_<int64_t>>> denseTiles;
    std::map<std::string, std::list<cv::Rect_<int64_t>>> bgTiles;

    std::string curImg;

protected:
    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    void customTiling();
    
public:
    // Performs the tiling using the current algorithm
    // Also used for mat tiling, instead of tiling form the svs image
    void tileMat(cv::Mat& mat, std::list<cv::Rect_<int64_t>>& tiles);

    BGPreTiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath, int64_t borders, CostFunction* cfunc, 
        BGMasker* bgm);

    std::map<std::string, std::list<cv::Rect_<int64_t>>> getDense() {
        return this->denseTiles;
    };
    std::map<std::string, std::list<cv::Rect_<int64_t>>> getBg() {
        return this->bgTiles;
    };
    long getBgSize() {
        long size = 0;
        for (std::pair<std::string, std::list<cv::Rect_<int64_t>>> p : this->bgTiles)
            size += p.second.size();
        return size;
    }
};

#endif // BG_PRE_TILED_RT_COLLECTION_H_
