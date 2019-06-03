#ifndef IRREG_TILED_RT_COLLECTION_H_
#define IRREG_TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "TiledRTCollection.h"
#include "costFuncs/BGMasker.h"

enum PreTilerAlg_t {
    NO_PRE_TILER,
    DENSE_BG_SEPARATOR,
};

enum TilerAlg_t {
    NO_TILER,
    LIST_ALG_HALF,
    LIST_ALG_EXPECT,
    KD_TREE_ALG_AREA,
    KD_TREE_ALG_COST,
};

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
