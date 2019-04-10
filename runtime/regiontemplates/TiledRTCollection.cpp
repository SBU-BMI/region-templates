#include "TiledRTCollection.h"

/*****************************************************************************/
/***************************** Helper functions ******************************/
/*****************************************************************************/

void osrRegionToCVMat(openslide_t* osr, cv::Rect_<int64_t> r, 
    int level, cv::Mat& thisTile) {

    uint32_t* osrRegion = new uint32_t[r.width * r.height];
    openslide_read_region(osr, osrRegion, r.x, r.y, level, r.width, r.height);

    thisTile = cv::Mat(r.height, r.width, CV_8UC3, cv::Scalar(0, 0, 0));
    int64_t numOfPixelPerTile = thisTile.total();

    for (int64_t it = 0; it < numOfPixelPerTile; ++it) {
        uint32_t p = osrRegion[it];

        uint8_t a = (p >> 24) & 0xFF;
        uint8_t r = (p >> 16) & 0xFF;
        uint8_t g = (p >> 8) & 0xFF;
        uint8_t b = p & 0xFF;

        switch (a) {
            case 0:
                r = 0;
                b = 0;
                g = 0;
                break;
            case 255:
                // no action needed
                break;
            default:
                r = (r * 255 + a / 2) / a;
                g = (g * 255 + a / 2) / a;
                b = (b * 255 + a / 2) / a;
                break;
        }

        // write back
        thisTile.at<cv::Vec3b>(it)[0] = b;
        thisTile.at<cv::Vec3b>(it)[1] = g;
        thisTile.at<cv::Vec3b>(it)[2] = r;
    }

    delete[] osrRegion;

    return;
}

int32_t getLargestLevel(openslide_t *osr) {
    int32_t levels = openslide_get_level_count(osr);
    int64_t w, h;

    int64_t maxSize = -1;
    int32_t maxLevel = -1;

    for (int32_t l=0; l<levels; l++) {
        openslide_get_level_dimensions(osr, l, &w, &h);
        if (h*w > maxSize) {
            maxSize = h*w;
            maxLevel = l;
        }
    }

    return maxLevel;
}

bool isSVS(std::string path) {
    std::size_t l = path.find_last_of(".");
    return path.substr(l+1).compare(".svs") == 0;
}

/*****************************************************************************/
/****************************** Class methods ********************************/
/*****************************************************************************/

TiledRTCollection::TiledRTCollection(std::string name, std::string tilesPath) {
    this->tiled = false;
    this->name = name;
    this->tilesPath = tilesPath;
}

TiledRTCollection::~TiledRTCollection() {

}

void TiledRTCollection::addImage(std::string path) {
    initialPaths.push_back(path);
}

RegionTemplate* TiledRTCollection::getRT(int id) {
    return rts[id];
}

// Template method hook for a custom tiling method.
// Defaults to returning the input images with a single
//   tile containing the full image.
void TiledRTCollection::customTiling() {
    // Go through all images
    for (int i=0; i<initialPaths.size(); i++) {
        bool isSvs = isSVS(initialPaths[i]);

        // Open image for tiling
        int64_t w = -1;
        int64_t h = -1;
        openslide_t* osr;
        int32_t osrMaxLevel = -1;
        cv::Mat mat;
        if (isSvs) {
            osr = openslide_open(initialPaths[i].c_str());
            osrMaxLevel = getLargestLevel(osr);
            openslide_get_level_dimensions(osr, osrMaxLevel, &w, &h);
        } else {
            mat = cv::imread(initialPaths[i]);
            h = mat.rows;
            w = mat.cols;
        }

        // Create the full roi
        cv::Rect_<int64_t> roi(0, 0, w, h);

        // Create a tile file
        cv::Mat tile;
        if (isSvs) {
            osrRegionToCVMat(osr, roi, osrMaxLevel, tile);
        } else {
            tile = mat(roi);
        }
        std::string path = tilesPath;
        path += "/i" + to_string(i) + TILE_EXT;
        cv::imwrite(path, tile);

        // Create new RT tile from roi
        DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
        ddr2d->setName(REF_DDR_NAME);
        ddr2d->setId(REF_DDR_NAME);
        ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
        ddr2d->setIsAppInput(true);
        ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
        ddr2d->setInputFileName(path);
        RegionTemplate* newRT = new RegionTemplate();
        newRT->insertDataRegion(ddr2d);
        newRT->setName(this->name);

        this->rts.push_back(newRT);

        // Close .svs file
        if (isSvs) {
            openslide_close(osr);
        }
    }
}

// Performs the autoTiler algorithm while updating the internal tiles 
//   representation std::map<int, std::vector<cv::Rect_<int64_t>>>
void TiledRTCollection::tileImages() {
    // A tiling process can only occur once
    if (this->tiled) {
        std::cout << "RT collection already tiled. Cannot re-tile it. " 
            << __FILE__ << ":" << __LINE__ << std::endl;
        exit(-1);
        // return;
    }

    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    customTiling();

    this->tiled = true;
}

// Performs the autoTiler algorithm while updating the internal tiles 
//   representation std::vector<std::list<cv::Rect_<int64_t>>>
void TiledRTCollection::tileImages(
    std::vector<std::list<cv::Rect_<int64_t>>> tiles) {

    // A tiling process can only occur once
    if (this->tiled) {
        std::cout << "RT collection already tiled. Cannot re-tile it. " 
            << __FILE__ << ":" << __LINE__ << std::endl;
        exit(-1);
        // return;
    }

    // The number of internal masks' paths and input tilings must match
    if (tiles.size() != initialPaths.size()) {
        std::cout << "Internal masks' paths and input tilings size mismatch. " 
            << __FILE__ << ":" << __LINE__ << std::endl;
        exit(-1);
        // return;
    }

    // Iterate through all input images indices
    for (int i=0; i<tiles.size(); i++) {
        bool isSvs = isSVS(initialPaths[i]);

        // Open image for tiling
        openslide_t* osr;
        int32_t osrMaxLevel = -1;
        cv::Mat mat;
        if (isSvs) {
            osr = openslide_open(initialPaths[i].c_str());
            osrMaxLevel = getLargestLevel(osr);
        } else {
            mat = cv::imread(initialPaths[i]);
        }

        // Go through all tiles
        int j = 0;
        for (cv::Rect_<int64_t> r : tiles[i]) {
            // Create a tile file
            cv::Mat tile;
            if (isSvs) {
                osrRegionToCVMat(osr, r, osrMaxLevel, tile);
            } else {
                tile = mat(r);
            }
            std::string path = tilesPath;
            path += "/i" + to_string(i) + "t" + to_string(j++) + TILE_EXT;
            cv::imwrite(path, tile);

            // Create new RT tile from ROI r
            DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
            ddr2d->setName(REF_DDR_NAME);
            ddr2d->setId(REF_DDR_NAME);
            ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
            ddr2d->setIsAppInput(true);
            ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
            ddr2d->setInputFileName(path);
            RegionTemplate* newRT = new RegionTemplate();
            newRT->insertDataRegion(ddr2d);
            newRT->setName(this->name);

            this->rts.push_back(newRT);
        }

        // Close .svs file
        if (isSvs) {
            openslide_close(osr);
        }
    }

    this->tiled = true;
}

std::vector<std::list<cv::Rect_<int64_t>>> TiledRTCollection::getTiles() {
    return tiles;
}
