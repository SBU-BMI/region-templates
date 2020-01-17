#ifndef BG_PRE_TILED_RT_COLLECTION_H_
#define BG_PRE_TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "TiledRTCollection.h"
#include "costFuncs/BGMasker.h"
#include "CostFunction.h"
#include "tilingAlgs/tilingUtil.h"
#include "tilingAlgs/denseFromBG.h"

class BGPreTiledRTCollection : public TiledRTCollection {
private:
    int border;
    BGMasker* bgm;

    std::list<rect_t> finalTiles;
    std::list<rect_t> denseTiles;

protected:
    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    void customTiling();

public:
    BGPreTiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath, int border, CostFunction* cfunc, BGMasker* bgm);
};

#endif // BG_PRE_TILED_RT_COLLECTION_H_
