#ifndef SINGLE_TILE_RT_COLLECTION_H_
#define SINGLE_TILE_RT_COLLECTION_H_

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

class SingleTileRTCollection : public TiledRTCollection,
                               public TiledMatCollection {
  private:
    long xi, xo, yi, yo;

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

    SingleTileRTCollection(std::string name, std::string refDDRName,
                           std::string tilesPath, long xi, long xo, long yi,
                           long yo);
};

#endif // HYBRID_DENSE_TILED_RT_COLLECTION_H_
