#include "RTCollectionTilingPipeline.h"
RTCollectionTilingPipeline::RTCollectionTilingPipeline(std::string rtName) {
    this->rtName = rtName;
}

void RTCollectionTilingPipeline::setImage(std::string imgPath) {
    this->imgPath = imgPath;
}

void RTCollectionTilingPipeline::setBorder(int64_t border) {
    this->border = border;
}

std::pair<std::string, RegionTemplate*> RTCollectionTilingPipeline::getRT(int id){
    return rts[id];
}

int RTCollectionTilingPipeline::getNumRTs() {
    return rts.size();
}

// Adds a tiling stage to the pipeline. The order matter.
void RTCollectionTilingPipeline::addStage(TilingStage* stage) {
    this->stages.push_back(stage);
}

// Full tiling execution. Can only be performed once
void RTCollectionTilingPipeline::tile() {
    // Can only execute tiling in svs files
    std::size_t l = this->imgPath.find_last_of(".");
    if (this->imgPath.substr(l).compare(".svs") != 0) {
        cout << "Tiling input image not svs." << endl;
        exit(-1);
    }

    // Opens the svs input file
    int64_t w = -1;
    int64_t h = -1;
    openslide_t* osr;
    int32_t osrMinLevel = -1;
    int32_t osrMaxLevel = 0; // svs standard: max level = 0
    float ratiow;
    float ratioh; 
    cv::Mat initialMat;

    osr = openslide_open(this->imgPath.c_str());

    // Gets info of largest image
    openslide_get_level0_dimensions(osr, &w, &h);
    ratiow = w;
    ratioh = h;

    // Opens smallest image as a cv mat
    osrMinLevel = openslide_get_level_count(osr) - 1; // last level
    openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
    cv::Rect_<int64_t> roi(0, 0, w, h);
    osrRegionToCVMat(osr, roi, osrMinLevel, initialMat);

    // Calculates the ratio between largest and smallest 
    // images' dimensions for later conversion
    ratiow /= w;
    ratioh /= h;

    // Creates the tile lists
    std::vector<cv::Rect_<int64_t>> unfinishedTiles;
    unfinishedTiles.push_back(cv::Rect_<int64_t>(0, 0, w, h)); // full img tile
    std::vector<cv::Rect_<int64_t>> finishedTiles;

    // Execute each stage of the pipeline
    for (TilingStage* s : stages) {
        uint64_t initTime = Util::ClockGetTime();
        s->tile(initialMat, unfinishedTiles, finishedTiles);
        uint64_t endTime = Util::ClockGetTime();
        std::cout << "[RTCollectionTilingPipeline] Stage " << s->getName() 
            << " executed in " << ((float)(endTime-initTime)/1000) 
            << " seconds." << std::endl;
    }

    finishedTiles.insert(finishedTiles.end(), 
        unfinishedTiles.begin(), unfinishedTiles.end());

    // Actually tile the image given the list of ROIs
    int drId=0;
    for (cv::Rect_<int64_t> tile : finishedTiles) {
        // Converts the tile roi for the bigger image
        tile.x *= ratiow;
        tile.width *= ratiow;
        tile.y *= ratioh;
        tile.height *= ratioh;

        // Creates the dr as a svs data region for
        // lazy read/write of input file
        DataRegion *dr = new DenseDataRegion2D();
        dr->setRoi(tile);
        dr->setSvs();
   
        // Create new RT tile from roi
        std::string drName = "t" + to_string(drId);
        dr->setName(drName);
        dr->setId("initial");
        dr->setInputType(DataSourceType::FILE_SYSTEM);
        dr->setIsAppInput(true);
        dr->setOutputType(DataSourceType::FILE_SYSTEM);
        dr->setInputFileName(this->imgPath);
        RegionTemplate* newRT = new RegionTemplate();
        newRT->insertDataRegion(dr);
        newRT->setName(this->rtName);

        // Add the tile and the RT to the internal containers
        this->tiles.push_back(tile);
        this->rts.push_back(
            std::pair<std::string, RegionTemplate*>(drName, newRT));
        drId++;
    }

    // Close .svs file
    openslide_close(osr);
}

// Copy tiling. Receives another tiling pipeline which has already 
// executed and applies the same tiling. Input images must be added
// in the same order. Internal border will not be used.
void RTCollectionTilingPipeline::tile(RTCollectionTilingPipeline* pipeline) {
    // Perform the tiling on the list of tiles from pipeline
    int drId=0;
    for (cv::Rect_<int64_t> tile : pipeline->tiles) {
        // Creates the dr as a svs data region for
        // lazy read/write of input file
        DataRegion *dr = new DenseDataRegion2D();
        dr->setRoi(tile);
        dr->setSvs();
   
        // Create new RT tile from roi
        std::string drName = "t" + to_string(drId);
        dr->setName(drName);
        dr->setId("initial");
        dr->setInputType(DataSourceType::FILE_SYSTEM);
        dr->setIsAppInput(true);
        dr->setOutputType(DataSourceType::FILE_SYSTEM);
        dr->setInputFileName(this->imgPath);
        RegionTemplate* newRT = new RegionTemplate();
        newRT->insertDataRegion(dr);
        newRT->setName(this->rtName);

        // Add the tile and the RT to the internal containers
        this->tiles.push_back(tile);
        this->rts.push_back(
            std::pair<std::string, RegionTemplate*>(drName, newRT));
        drId++;
    }
}
