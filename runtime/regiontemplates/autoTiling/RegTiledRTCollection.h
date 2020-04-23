#ifndef REG_TILED_RT_COLLECTION_H_
#define REG_TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "TiledRTCollection.h"
#include "TiledMatCollection.h"
#include "tilingAlgs/tilingUtil.h"
#include "tilingAlgs/fixedGrid.h"
#include "CostFunction.h"

class RegTiledRTCollection : public TiledRTCollection, public TiledMatCollection {
private:
    int64_t tw;
    int64_t th;
    int64_t nTiles;

protected:
    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    void customTiling();

    // Performs the tiling using the current algorithm
    // Also used for mat tiling, instead of tiling form the svs image
    void tileMat(cv::Mat& mat, std::list<cv::Rect_<int64_t>>& tiles);

public:
    RegTiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath, int64_t tw, int64_t th, int64_t borders,
        CostFunction* cfunc);
    RegTiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath, int64_t nTiles, int64_t borders,
        CostFunction* cfunc);
};

#endif
