#ifndef HYBRID_DENSE_TILED_RT_COLLECTION_H_
#define HYBRID_DENSE_TILED_RT_COLLECTION_H_

#include <list>
#include <string>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "CostFunction.h"
#include "TiledMatCollection.h"
#include "TiledRTCollection.h"
#include "costFuncs/BGMasker.h"
#include "tilingAlgs/fixedGrid.h"
#include "tilingAlgs/kdTreeCutting.h"
#include "tilingAlgs/listCutting.h"
#include "tilingAlgs/quadTreeCutting.h"
#include "tilingAlgs/tilingUtil.h"

class HybridDenseTiledRTCollection : public TiledRTCollection,
                                     public TiledMatCollection {
  private:
    int   nCpuTiles; // Expected number of dense tiles by the end of te tiling
    int   nGpuTiles; // Expected number of dense tiles by the end of te tiling
    float cpuPATS;
    float gpuPATS;
    BGMasker *bgm;

    bool fgBgRem;

    PreTilerAlg_t preTier;
    TilerAlg_t    tilingAlg;

  protected:
    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    void customTiling();

  public:
    // Performs the tiling using the current algorithm
    // Also used for mat tiling, instead of tiling form the svs image
    void tileMat(cv::Mat &mat, std::list<cv::Rect_<int64_t>> &tiles,
                 std::list<cv::Rect_<int64_t>> &bgTiles);

    HybridDenseTiledRTCollection(std::string name, std::string refDDRName,
                                 std::string tilesPath, int64_t borders,
                                 CostFunction *cfunc, BGMasker *bgm,
                                 TilerAlg_t tilingAlg = FIXED_GRID_TILING,
                                 int nCpuTiles = 1, int nGpuTiles = 1,
                                 float cpuPATS = 1.0, float gpuPATS = 1.0,
                                 bool fgBgRem = false);
};

#endif // HYBRID_DENSE_TILED_RT_COLLECTION_H_
