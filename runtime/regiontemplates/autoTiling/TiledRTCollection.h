#ifndef TILED_RT_COLLECTION_H_
#define TILED_RT_COLLECTION_H_

#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "CostFunction.h"
#include "RegionTemplate.h"
#include "svs/svsUtils.h"
#include "tilingAlgs/tilingUtil.h"

static const std::string TILE_EXT = ".tiff";

// Should use ExecEngineConstants::GPU ...
typedef int Target_t;

class TiledRTCollection {
  private:
    bool tiled;
    bool drGen;

  protected:
    int64_t borders;
    int64_t nTiles;

    std::string              name;
    std::string              refDDRName;
    std::string              tilesPath;
    std::vector<std::string> initialPaths;
    cv::Rect_<int64_t>       imageSize;

    // vector<pair<DR name, actual RT object with a single DR>
    std::vector<std::pair<std::string, RegionTemplate *>> rts;

    // used only for hybrid tiling
    std::vector<Target_t> tileTarget;

    // Cost function for profiling the final tiles generated
    CostFunction *cfunc;

    // Flag to signal if the input images were pre-tiles.
    // If so, dense tiling should begin from a filled 'tiles' with the
    // previous values.
    bool preTiled;

    // Map of <InputImageStr,TilesList>
    std::map<std::string, std::list<cv::Rect_<int64_t>>> tiles;

    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    virtual void customTiling();

  public:
    TiledRTCollection(std::string name, std::string refDDRName,
                      std::string tilesPath, int64_t borders,
                      CostFunction *cfunc, int64_t nTiles = -1);
    ~TiledRTCollection();

    void                                     addImage(std::string path);
    std::pair<std::string, RegionTemplate *> getRT(int id);
    Target_t                                 getTileTarget(int id);
    int                                      getNumRTs() { return rts.size(); }

    // Can only be called once
    void tileImages(bool tilingOnly = false);

    // Must be called only on the last level of the tiling pipeline
    void generateDRs(bool tilingOnly = false);

    // Can only be called once
    void
    setPreTiles(std::map<std::string, std::list<cv::Rect_<int64_t>>> tiles);
    void addTiles(std::map<std::string, std::list<cv::Rect_<int64_t>>> tiles);
    void addTargets(std::vector<Target_t> targets);

    std::vector<std::list<cv::Rect_<int64_t>>>           getTiles();
    std::map<std::string, std::list<cv::Rect_<int64_t>>> getTilesBase() {
        return tiles;
    };

    cv::Rect_<int64_t> getImageSize() { return imageSize; };

    std::vector<Target_t> getTargetsBase() { return tileTarget; };
};

#endif
