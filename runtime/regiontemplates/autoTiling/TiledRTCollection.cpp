#include "TiledRTCollection.h"

/*****************************************************************************/
/****************************** Class methods ********************************/
/*****************************************************************************/

TiledRTCollection::TiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath) {

    this->tiled = false;
    this->name = name;
    this->refDDRName = refDDRName;
    this->tilesPath = tilesPath;
    this->lazyTiling = false;
}

TiledRTCollection::~TiledRTCollection() {
    cleanup(this->tilesPath + "/" + name);
}

void TiledRTCollection::addImage(std::string path) {
    initialPaths.push_back(path);
}

std::pair<std::string, RegionTemplate*> TiledRTCollection::getRT(int id) {
    return rts[id];
}

// Template method hook for a custom tiling method.
// Defaults to returning the input images with a single
//   tile containing the full image.
void TiledRTCollection::customTiling() {
    // Only a single name is required since there is only one tile
    std::string drName = to_string(0);

    // Go through all images
    for (int i=0; i<initialPaths.size(); i++) {
        bool isSvs = isSVS(initialPaths[i]);

        // Open image for tiling
        int64_t w = -1;
        int64_t h = -1;
        openslide_t* osr;
        int32_t osrMaxLevel = 0; // svs standard: max level = 0
        cv::Mat mat;
        if (isSvs) {
            osr = openslide_open(initialPaths[i].c_str());
            openslide_get_level0_dimensions(osr, &w, &h);
        } else {
            mat = cv::imread(initialPaths[i]);
            h = mat.rows;
            w = mat.cols;
        }

        // Create the full roi and add it to the roi's vector
        cv::Rect_<int64_t> roi(0, 0, w, h);
        cv::Rect_<int64_t> rois[] = {roi};
        this->tiles.push_back(std::list<cv::Rect_<int64_t>>(rois, 
            rois + sizeof(rois)/sizeof(cv::Rect_<int64_t>)));

        // Create a tile file
        cv::Mat tile;
        if (isSvs) {
            osrRegionToCVMat(osr, roi, osrMaxLevel, tile);
        } else {
            tile = mat(roi);
        }
        std::string path = this->tilesPath + "/" + name + "/";
        path += "/i" + to_string(i) + TILE_EXT;
        cv::imwrite(path, tile);

        // Create new RT tile from roi
        DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
        ddr2d->setName(drName);
        ddr2d->setId(refDDRName);
        ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
        ddr2d->setIsAppInput(true);
        ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
        ddr2d->setInputFileName(path);
        RegionTemplate* newRT = new RegionTemplate();
        newRT->setName(this->name);
        newRT->insertDataRegion(ddr2d);

        this->rts.push_back(
            std::pair<std::string, RegionTemplate*>(drName, newRT));

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
        cleanup(this->tilesPath + "/" + name);
        exit(-1);
        // return;
    }

    if (!this->lazyTiling) {
        std::string cmd = "mkdir " + this->tilesPath + "/" + this->name;
        const int dir_err = system(cmd.c_str());
        if (dir_err == -1) {
            std::cout << "Error creating directory. " << __FILE__ << ":" 
                << __LINE__ << std::endl;
            exit(1);
        }
    }

    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    customTiling();

    this->tiled = true;
}

// Performs the tiling using a previously tiled TRTC
void TiledRTCollection::tileImages(
    std::vector<std::list<cv::Rect_<int64_t>>> tiles) {

    if (!this->lazyTiling) {
        std::string cmd = "mkdir " + this->tilesPath + "/" + this->name;
        const int dir_err = system(cmd.c_str());
        if (dir_err == -1) {
            std::cout << "Error creating directory. " << __FILE__ << ":" 
                << __LINE__ << std::endl;
            exit(1);
        }
    }

    // Only a single name is required since there is only one tile
    std::string drName = to_string(0);

    // A tiling process can only occur once
    if (this->tiled) {
        std::cout << "RT collection already tiled. Cannot re-tile it. " 
            << __FILE__ << ":" << __LINE__ << std::endl;
        cleanup(this->tilesPath + "/" + name);
        exit(-1);
        // return;
    }

    // The number of internal masks' paths and input tilings must match
    if (tiles.size() != initialPaths.size()) {
        std::cout << "Internal masks' paths and input tilings size mismatch. "
            << "Expected "  << initialPaths.size() << " but got " << tiles.size()
            << ". " << __FILE__ << ":" << __LINE__ << std::endl;
        cleanup(this->tilesPath + "/" + name);
        exit(-1);
        // return;
    }

    // Iterate through all input images indices 
    for (int i=0; i<tiles.size(); i++) {
        bool isSvs = isSVS(initialPaths[i]);

        // Open image for tiling
        openslide_t* osr;
        int32_t osrMaxLevel = 0; // svs standard: max level = 0
        cv::Mat mat;
        if (isSvs) {
            osr = openslide_open(initialPaths[i].c_str());
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
            std::string path = this->tilesPath + name;
            path += "/i" + to_string(i) + "t" + to_string(j++) + TILE_EXT;
            cv::imwrite(path, tile);

            // Create new RT tile from ROI r
            DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
            ddr2d->setName(drName);
            ddr2d->setId(refDDRName);
            ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
            ddr2d->setIsAppInput(true);
            ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
            ddr2d->setInputFileName(path);
            RegionTemplate* newRT = new RegionTemplate();
            newRT->insertDataRegion(ddr2d);
            newRT->setName(this->name);

            this->rts.push_back(
                std::pair<std::string, RegionTemplate*>(drName, newRT));
        }

        // Close .svs file
        if (isSvs) {
            openslide_close(osr);
        }
    }

    this->tiled = true;
}

std::vector<std::list<cv::Rect_<int64_t>>> TiledRTCollection::getTiles() {
    if (!this->tiled) {
        std::cout << "Tiles not yet generated. " 
            << __FILE__ << ":" << __LINE__ << std::endl;
        cleanup(this->tilesPath + "/" + name);
        exit(-1);
    }
    return tiles;
}

/*****************************************************************************/
/***************************** Helper functions ******************************/
/*****************************************************************************/

void cleanup(std::string path) {
    std::string cmd = "rm -rf " + path;
    const int dir_err = system(cmd.c_str());
    if (dir_err == -1) {
        std::cout << "Error cleaning up. " << __FILE__ 
            << ":" << __LINE__ << std::endl;
        exit(1);
    }
}
