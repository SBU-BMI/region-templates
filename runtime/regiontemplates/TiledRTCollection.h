#ifndef TILED_RT_COLLECTION_H_
#define TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "RegionTemplate.h"

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
    
    // If true, tiles files won't be generated
    // Only usable with svs inputs
    bool lazyTiling;

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

    void setLazyReading() {this->lazyTiling = true;}

};

/*****************************************************************************/
/***************************** Helper functions ******************************/
/*****************************************************************************/

void cleanup(std::string path);
bool isSVS(std::string path);
int32_t getLargestLevel(openslide_t *osr);
int32_t getSmallestLevel(openslide_t *osr);
void osrRegionToCVMat(openslide_t* osr, cv::Rect_<int64_t> r, 
    int level, cv::Mat& thisTile);

#endif
