#include "BGPreTiledRTCollection.h"

BGPreTiledRTCollection::BGPreTiledRTCollection(
    std::string name, std::string refDDRName, std::string tilesPath,
    int64_t borders, CostFunction *cfunc, BGMasker *bgm)
    : TiledRTCollection(name, refDDRName, tilesPath, borders, cfunc) {

    this->bgm    = bgm;
    this->curImg = "single";
}

// May break when using more than one input image since DR id is unique
// only between input images.
void BGPreTiledRTCollection::customTiling() {
    std::string drName;
    // Go through all images
    for (std::string img : this->initialPaths) {
        // Open image for tiling
        int64_t      w = -1;
        int64_t      h = -1;
        openslide_t *osr;
        int32_t      osrMinLevel = -1;
        cv::Mat      maskMat;

        // Opens svs input file
        osr = openslide_open(img.c_str());

        // Opens smallest image as a cv mat
        osrMinLevel = openslide_get_level_count(osr) - 1; // last level
        openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
        cv::Rect_<int64_t> roi(0, 0, w, h);
        osrRegionToCVMat(osr, roi, osrMinLevel, maskMat);

        // Close .svs file
        openslide_close(osr);

        this->curImg = img;
        this->tileMat(maskMat, this->tiles[img], this->bgTiles[img]);
    }
}

void BGPreTiledRTCollection::tileMat(cv::Mat                       &mat,
                                     std::list<cv::Rect_<int64_t>> &tiles,
                                     std::list<cv::Rect_<int64_t>> &bgTiles2) {

    // Gets threshold mask
    cv::Mat thMask = bgm->bgMask(mat);

    // Performs tiling
    std::list<rect_t> denseTiles;
    std::list<rect_t> bgTiles;
    tileDenseFromBG(thMask, denseTiles, bgTiles, &mat);

    // Convert rect_t to cv::Rect_ and add to output lists
    tiles.clear();
    std::list<cv::Rect_<int64_t>> &cvDenseTiles = tiles;
    for (std::list<rect_t>::iterator r = denseTiles.begin();
         r != denseTiles.end(); r++) {

        if (r->xo - r->xi > 0 && r->yo - r->yi)
            cvDenseTiles.push_back(
                cv::Rect_<int64_t>(r->xi, r->yi, r->xo - r->xi, r->yo - r->yi));
    }
    this->denseTiles[this->curImg] = cvDenseTiles;

    // Convert rect_t to cv::Rect_ and add to output lists
    std::list<cv::Rect_<int64_t>> &cvBgTiles = bgTiles2;
    for (std::list<rect_t>::iterator r = bgTiles.begin(); r != bgTiles.end();
         r++) {

        if (r->xo - r->xi > 0 && r->yo - r->yi)
            cvBgTiles.push_back(
                cv::Rect_<int64_t>(r->xi, r->yi, r->xo - r->xi, r->yo - r->yi));
    }
    // Sort bg tiles by size, i.e., load cost
    cvBgTiles.sort(
        [](const cv::Rect_<int64_t> &l, const cv::Rect_<int64_t> &r) {
            return l.width * l.height > r.width * r.height;
        });
    this->bgTiles[this->curImg] = cvBgTiles;

    std::cout << "[BGPreTiledRTCollection] Dense tiles: " << tiles.size()
              << ", bg tiles: " << bgTiles2.size() << std::endl;

    // // Add all tiles to final compiled list
    // tiles.clear();
    // tiles.insert(tiles.begin(), cvDenseTiles.begin(), cvDenseTiles.end());
    // bgTiles2.insert(bgTiles2.begin(), cvBgTiles.begin(), cvBgTiles.end());
}
