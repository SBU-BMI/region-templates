#ifndef IRREG_TILED_RT_COLLECTION_H_
#define IRREG_TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "TiledRTCollection.h"
#include "TiledMatCollection.h"
#include "costFuncs/BGMasker.h"
#include "CostFunction.h"
#include "tilingAlgs/tilingUtil.h"
#include "tilingAlgs/listCutting.h"
#include "tilingAlgs/kdTreeCutting.h"
#include "tilingAlgs/quadTreeCutting.h"

class IrregTiledRTCollection : public TiledRTCollection, public TiledMatCollection {
private:
    int nTiles; // Expected number of dense tiles by the end of te tiling
    BGMasker* bgm;

    PreTilerAlg_t preTier;
    TilerAlg_t tilingAlg;

protected:
    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    void customTiling();

public:
    // Performs the tiling using the current algorithm
    // Also used for mat tiling, instead of tiling form the svs image
    void tileMat(cv::Mat& mat, std::list<cv::Rect_<int64_t>>& tiles);

    IrregTiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath, int64_t borders, CostFunction* cfunc, 
        BGMasker* bgm, TilerAlg_t tilingAlg=FIXED_GRID_TILING, int nTiles=0);
};

#endif
