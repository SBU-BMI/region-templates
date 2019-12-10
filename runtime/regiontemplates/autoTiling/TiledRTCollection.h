#ifndef TILED_RT_COLLECTION_H_
#define TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "RegionTemplate.h"
#include "svs/svsUtils.h"
#include "tilingAlgs/tilingUtil.h"

static const std::string TILE_EXT = ".tiff";

class TiledRTCollection {
private:
    bool tiled;

protected:
    std::string name;
    std::string refDDRName;
    std::string tilesPath;
    std::vector<std::string> initialPaths;
    std::vector<std::list<cv::Rect_<int64_t> > > tiles;
    // vector<pair<DR name, actual RT object with the only DR>
    std::vector<std::pair<std::string, RegionTemplate*> > rts;
    
    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    virtual void customTiling();

public:
    TiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath);
    ~TiledRTCollection();

    void addImage(std::string path);
    std::pair<std::string, RegionTemplate*> getRT(int id);
    int getNumRTs() {
        return rts.size();
    }

    void tileImages();

    // tiling methods for mask tiling, based on a previous tiling
    void tileImages(std::vector<std::list<cv::Rect_<int64_t>>> tiles);
    std::vector<std::list<cv::Rect_<int64_t>>> getTiles();

};

/*****************************************************************************/
/***************************** Helper functions ******************************/
/*****************************************************************************/

void cleanup(std::string path);

#endif
