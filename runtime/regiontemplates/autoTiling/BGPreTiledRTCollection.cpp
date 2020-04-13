#include "BGPreTiledRTCollection.h"

BGPreTiledRTCollection::BGPreTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int64_t borders, 
    CostFunction* cfunc, BGMasker* bgm) 
        : TiledRTCollection(name, refDDRName, tilesPath, borders, cfunc) {

    this->bgm = bgm;
}

// May break when using more than one input image since DR id is unique
// only between input images.
void BGPreTiledRTCollection::customTiling() {
    std::string drName;
    // Go through all images
    for (std::string img : this->initialPaths) {
        // Open image for tiling
        int64_t w = -1;
        int64_t h = -1;
        openslide_t* osr;
        int32_t osrMinLevel = -1;
        cv::Mat maskMat;

        // Opens svs input file
        osr = openslide_open(img.c_str());

        // Opens smallest image as a cv mat
        osrMinLevel = openslide_get_level_count(osr) - 1; // last level
        openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
        cv::Rect_<int64_t> roi(0, 0, w, h);
        osrRegionToCVMat(osr, roi, osrMinLevel, maskMat);

        // Close .svs file
        openslide_close(osr);

        // Gets threshold mask
        cv::Mat thMask = bgm->bgMask(maskMat);
        
        // Performs tiling
        std::list<rect_t> denseTiles;
        std::list<rect_t> bgTiles;
        tileDenseFromBG(thMask, denseTiles, bgTiles, &maskMat);

        std::cout << "[BGPreTiledRTCollection] Dense tiles: " 
            << denseTiles.size() << ", bg tiles: " 
            << bgTiles.size() << std::endl;

        // Convert rect_t to cv::Rect_ and add to output lists
        std::list<cv::Rect_<int64_t> > cvDenseTiles;
        for (std::list<rect_t>::iterator r=denseTiles.begin(); 
                r!=denseTiles.end(); r++) {

            cvDenseTiles.push_back(cv::Rect_<int64_t>(
                r->xi, r->yi, r->xo-r->xi, r->yo-r->yi));
        }
        this->denseTiles[img] = cvDenseTiles;

        // Convert rect_t to cv::Rect_ and add to output lists
        std::list<cv::Rect_<int64_t> > cvBgTiles;
        for (std::list<rect_t>::iterator r=bgTiles.begin(); 
                r!=bgTiles.end(); r++) {

            cvBgTiles.push_back(cv::Rect_<int64_t>(
                r->xi, r->yi, r->xo-r->xi, r->yo-r->yi));
        }
        this->bgTiles[img] = cvBgTiles;

        // Add all tiles to final compiled list
        this->tiles[img.c_str()] = std::list<cv::Rect_<int64_t>>();
        this->tiles[img.c_str()].insert(this->tiles[img.c_str()].end(), 
            cvDenseTiles.begin(), cvDenseTiles.end());
        this->tiles[img.c_str()].insert(this->tiles[img.c_str()].end(), 
            cvBgTiles.begin(), cvBgTiles.end());
    }
}
