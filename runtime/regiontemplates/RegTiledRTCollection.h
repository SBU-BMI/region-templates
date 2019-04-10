#ifndef REG_TILED_RT_COLLECTION_H_
#define REG_TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "TiledRTCollection.h"
#include "RegionTemplate.h"

class RegTiledRTCollection : public TiledRTCollection {
private:
    // std::string name;
    // std::string refDDRName;
    // std::string tilesPath;
    // bool tiled;
    // std::vector<std::list<cv::Rect_<int64_t>>> tiles;
    // std::vector<RegionTemplate*> rts;
    // std::vector<std::string> initialPaths;
    int64_t tw;
    int64_t th;

protected:
    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    void customTiling();

public:
    RegTiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath, int64_t tw, int64_t th);
    // ~RegTiledRTCollection();

    // void addImage(std::string path);
    // RegionTemplate* getRT(int id);
    // int getNumRTs() {
    //     return rts.size();
    // }

    // void tileImages();

    // // tiling methods for mask tiling, based on a previous tiling
    // void tileImages(std::vector<std::list<cv::Rect_<int64_t>>> tiles);
    // std::vector<std::list<cv::Rect_<int64_t>>> getTiles();

};

#endif
