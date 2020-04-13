#include "RegTiledRTCollection.h"

RegTiledRTCollection::RegTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int64_t tw, int64_t th, 
    int64_t borders, CostFunction* cfunc) 
        : TiledRTCollection(name, refDDRName, tilesPath, borders, cfunc) {

    if (this->borders > tw || this->borders > th) {
        std::cout << "Border cannot be greater than a tile's width" 
            << std::endl;
        exit(-2);
    }

    this->tw = tw;
    this->th = th;
    this->nTiles = 0;
}

RegTiledRTCollection::RegTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int64_t nTiles, 
    int64_t borders, CostFunction* cfunc) 
        : TiledRTCollection(name, refDDRName, tilesPath, borders, cfunc) {

    this->nTiles = nTiles;
}

void RegTiledRTCollection::customTiling() {
    // Go through all images
    for (std::string img : this->initialPaths) {
        // Open image for tiling
        int64_t w = -1;
        int64_t h = -1;
        openslide_t* osr;
        int32_t osrMinLevel = -1;
        cv::Mat mat;

        // Opens svs input file
        osr = openslide_open(img.c_str());

        // Opens smallest image as a cv mat
        osrMinLevel = openslide_get_level_count(osr) - 1; // last level
        openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
        cv::Rect_<int64_t> roi(0, 0, w, h);
        osrRegionToCVMat(osr, roi, osrMinLevel, mat);

        // Close .svs file
        openslide_close(osr);

        // Calculates the tiles sizes given the nTiles
        std::list<cv::Rect_<int64_t>> curTiles;
        if (this->nTiles <= 1) {
            cv::Rect_<int64_t> r;
            r.x = 0;
            r.width = w;
            r.y = 0;
            r.height = h;
            
            // Adds single tile
            curTiles.push_back(r);
        } else {
            fixedGrid(this->nTiles, w, h, 0, 0, curTiles);
        }

        this->tiles[img.c_str()] = curTiles;
    }
}
