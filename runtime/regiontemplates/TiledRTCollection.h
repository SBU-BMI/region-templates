#ifndef TILED_RT_COLLECTION_H_
#define TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "RegionTemplate.h"

static const std::string REF_DDR_NAME = "initial";
static const std::string TILE_EXT = ".tiff";

class TiledRTCollection {
private:
    std::string name;
    std::string tilesPath;
    bool tiled;
    std::vector<std::list<cv::Rect_<int64_t>>> tiles;
    std::vector<RegionTemplate*> rts;
    std::vector<std::string> initialPaths;

protected:
    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    virtual void customTiling();

public:
    TiledRTCollection(std::string name, std::string tilesPath);
    ~TiledRTCollection();

    void addImage(std::string path);
    RegionTemplate* getRT(int id);
    int getNumRTs() {
        return rts.size();
    }

    void tileImages();

    // tiling methods for mask tiling, based on a previous tiling
    void tileImages(std::vector<std::list<cv::Rect_<int64_t>>> tiles);
    std::vector<std::list<cv::Rect_<int64_t>>> getTiles();

};

#endif
