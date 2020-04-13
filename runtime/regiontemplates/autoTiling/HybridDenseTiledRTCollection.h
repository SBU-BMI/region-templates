#ifndef HYBRID_DENSE_TILED_RT_COLLECTION_H_
#define HYBRID_DENSE_TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "TiledRTCollection.h"
#include "costFuncs/BGMasker.h"
#include "CostFunction.h"
#include "tilingAlgs/tilingUtil.h"
#include "tilingAlgs/fixedGrid.h"
#include "tilingAlgs/listCutting.h"
#include "tilingAlgs/kdTreeCutting.h"
#include "tilingAlgs/quadTreeCutting.h"

class HybridDenseTiledRTCollection : public TiledRTCollection {
private:
    int nCpuTiles; // Expected number of dense tiles by the end of te tiling
    int nGpuTiles; // Expected number of dense tiles by the end of te tiling
    float cpuPATS;
    float gpuPATS;
    BGMasker* bgm;

    PreTilerAlg_t preTier;
    TilerAlg_t tilingAlg;

protected:
    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    void customTiling();

public:
    HybridDenseTiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath, int64_t borders, CostFunction* cfunc, 
        BGMasker* bgm, TilerAlg_t tilingAlg=FIXED_GRID_TILING, int nCpuTiles=1, 
        int nGpuTiles=1, float cpuPATS=1.0, float gpuPATS=1.0);
};

#endif // HYBRID_DENSE_TILED_RT_COLLECTION_H_
