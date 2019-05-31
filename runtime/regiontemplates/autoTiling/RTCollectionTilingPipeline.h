#ifndef RT_COLLECTION_TILING_PIPELINE_H_
#define RT_COLLECTION_TILING_PIPELINE_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

// #include "openslide.h"

// #include "TiledRTCollection.h"
// #include "costFuncs/BGMasker.h"

#include "RegionTemplate.h"
#include "TilingStage.h"

class RTCollectionTilingPipeline {
private:
    std::string imgPath;
    std::string rtName;
    int64_t border;

    // List of stages of the pipeline
    std::list<TilingStage*> stages;

    // Tiles after full execution of the pipeline
    std::vector<cv::Rect_<int64_t> > tiles;

    // vector<pair<DR name, actual RT object with the only DR>
    std::vector<std::pair<std::string, RegionTemplate*> > rts;

public:
    RTCollectionTilingPipeline(std::string rtName);

    // Parameter setting methods
    void setImage(std::string imgPath);
    void setBorder(int64_t border);

    std::pair<std::string, RegionTemplate*> getRT(int id);
    int getNumRTs();

    // Adds a tiling stage to the pipeline. The order matter.
    void addStage(TilingStage* stage);

    // Full tiling execution. Can only be performed once
    void tile();

    // Copy tiling. Receives another tiling pipeline which has already 
    // executed and applies the same tiling. Input images must be added
    // in the same order. Internal border will not be used.
    void tile(RTCollectionTilingPipeline* pipeline);
};

#endif // RT_COLLECTION_TILING_PIPELINE_H_
