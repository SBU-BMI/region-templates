#ifndef IRREG_TILED_RT_COLLECTION_H_
#define IRREG_TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "TiledRTCollection.h"
#include "costFuncs/BGMasker.h"
#include "tilingAlgs/tilingUtil.h"
#include "tilingAlgs/denseFromBG.h"
#include "tilingAlgs/listCutting.h"
#include "tilingAlgs/kdTreeCutting.h"

class IrregTiledRTCollection : public TiledRTCollection {
private:
    int border;
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
    IrregTiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath, int border, BGMasker* bgm, 
        PreTilerAlg_t preTier=NO_PRE_TILER, TilerAlg_t tilingAlg=NO_TILER, 
        int nTiles=0);
};

#endif
