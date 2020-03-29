#ifndef REG_TILED_RT_COLLECTION_H_
#define REG_TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "TiledRTCollection.h"
#include "tilingAlgs/tilingUtil.h"
#include "tilingAlgs/fixedGrid.h"
#include "CostFunction.h"

class RegTiledRTCollection : public TiledRTCollection {
private:
    int64_t tw;
    int64_t th;
    int64_t border;
    int64_t nTiles;

protected:
    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    void customTiling();

public:
    RegTiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath, int64_t tw, int64_t th, int64_t border,
        CostFunction* cfunc);
    RegTiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath, int64_t nTiles, int64_t border,
        CostFunction* cfunc);
};

#endif
