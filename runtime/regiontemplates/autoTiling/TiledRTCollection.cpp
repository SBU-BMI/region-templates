#include "TiledRTCollection.h"

/*****************************************************************************/
/****************************** Class methods ********************************/
/*****************************************************************************/

TiledRTCollection::TiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, CostFunction* cfunc) {

    this->tiled = false;
    this->name = name;
    this->refDDRName = refDDRName;
    this->tilesPath = tilesPath;
    this->cfunc = cfunc;
}

TiledRTCollection::~TiledRTCollection() {
    // cleanup(this->tilesPath + "/" + name);
}

void TiledRTCollection::addImage(std::string path) {
    initialPaths.push_back(path);
}

std::pair<std::string, RegionTemplate*> TiledRTCollection::getRT(int id) {
    return rts[id];
}

Target_t TiledRTCollection::getTileTarget(int id) {
    if (this->tileTarget.size() == 0)
        return ExecEngineConstants::CPU;
    else
        return this->tileTarget[id];
}

// Template method hook for a custom tiling method.
// Defaults to returning the input images with a single
//   tile containing the full image.
void TiledRTCollection::customTiling() {
    // Only a single name is required since there is only one tile
    std::string drName = "t0";

    // Go through all images
    for (int i=0; i<initialPaths.size(); i++) {
        
        cv::Mat tile;
        openslide_t* osr;

        // Checks file extension
        if (!isSVS(initialPaths[i])) {
            tile = cv::imread(initialPaths[i]);
        } else {
            // Open svs image for tiling
            int64_t w = -1;
            int64_t h = -1;
            int32_t osrMaxLevel = 0; // svs standard: max level = 0
            osr = openslide_open(initialPaths[i].c_str());
            openslide_get_level0_dimensions(osr, &w, &h);

            // Create the tile mat
            cv::Rect_<int64_t> roi(0, 0, w, h);
            osrRegionToCVMat(osr, roi, osrMaxLevel, tile);
        }

        // Create the full roi and add it to the roi's vector
        cv::Rect_<int64_t> roi(0, 0, tile.cols, tile.rows);
        cv::Rect_<int64_t> rois[] = {roi};
        this->tiles.push_back(std::list<cv::Rect_<int64_t>>(rois, 
            rois + sizeof(rois)/sizeof(cv::Rect_<int64_t>)));

        // Create new RT tile from roi
        DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
        ddr2d->setName(drName);
        ddr2d->setId(this->refDDRName);
        ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
        ddr2d->setIsAppInput(true);
        ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
        ddr2d->setInputFileName(initialPaths[i]);
        
        // Sets svs ddr parameters and closes svs image
        if (isSVS(initialPaths[i])) {
            ddr2d->setSvs();
            ddr2d->setRoi(roi);
            openslide_close(osr);
        }

        RegionTemplate* newRT = new RegionTemplate();
        newRT->setName(this->name);
        newRT->insertDataRegion(ddr2d);

        this->rts.push_back(
            std::pair<std::string, RegionTemplate*>(drName, newRT));
    }
}

#define CREATE_TILES

// Performs the autoTiler algorithm while updating the internal tiles 
//   representation std::map<int, std::vector<cv::Rect_<int64_t>>>
void TiledRTCollection::tileImages(bool tilingOnly) {
    // A tiling process can only occur once
    if (this->tiled) {
        std::cout << "RT collection already tiled. Cannot re-tile it. " 
            << __FILE__ << ":" << __LINE__ << std::endl;
        // cleanup(this->tilesPath + "/" + name);
        exit(-1);
        // return;
    }

    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    customTiling();

    this->tiled = true;

    if (tilingOnly) {
        for (int i=0; i<this->getTiles().size(); i++) {

            cv::Mat baseImg;
            if (isSVS(initialPaths[i]))
                osrFilenameToCVMat(this->initialPaths[i], baseImg);
            else
                baseImg = cv::imread(this->initialPaths[i]);
            
            // get cost image
            cv::Mat tiledImg;
            tiledImg = this->cfunc->costImg(baseImg);
            cv::cvtColor(tiledImg, tiledImg, cv::COLOR_GRAY2RGB);

            #ifdef CREATE_TILES
            int id=0;
            #endif

            // For each tile of the current image
            for (cv::Rect_<int64_t> tile : tiles[i]) {
                // Print tile with readable unber format
                setlocale(LC_NUMERIC, "en_US.utf-8");
                char cost[50];
                sprintf(cost, "%'2ld", this->cfunc->cost(baseImg, tile));
                std::cout << "\ttile " << tile.x << ":" << tile.width
                    << "\t" << tile.y << ":" << tile.height << "\tcost: " << 
                    cost << std::endl;

                // Adds tile rectangle region to tiled image
                cv::rectangle(tiledImg, 
                    cv::Point(tile.x,tile.y), 
                    cv::Point(tile.x+tile.width,
                              tile.y+tile.height),
                    (255,255,255),5);

                // Add cost to image as text
                cv::putText(tiledImg, 
                    cost,
                    cv::Point(tile.x+10, tile.y+tile.height/2),
                    cv::FONT_HERSHEY_SIMPLEX, 3, (255,255,255), 7);

                #ifdef CREATE_TILES
                // create tile image
                cv::Mat tileMat = cv::Mat(baseImg, cv::Rect(tile.x, tile.y, 
                    tile.width, tile.height));
                std::string outname = "./tiles/t" + std::to_string(id++) + ".png";
                cv::imwrite(outname, tileMat);
                #endif // CREATE_TILES
            }

            std::string outname = "./tiled-" + this->initialPaths[i] + ".png";
            cv::imwrite(outname, tiledImg);
        }
    }

#define DEBUG
#define PROFILING2

#ifdef DEBUG
    std::cout << "==== format: tile x:width, y:height" << std::endl;
#endif

#ifdef PROFILING2
    // Creates a list of costs for each tile for each image
    std::list<int64_t> costs;
    // Creates a list of perimeters for each tile for each image
    std::list<int64_t> perims;
#endif // #ifdef PROFILING2

#if defined(DEBUG) || defined(PROFILING2)
    // For each image tiled
    for (int i=0; i<this->getTiles().size(); i++) {

#ifdef DEBUG
        std::cout << "image " << i << std::endl;
#endif // #ifdef DEBUG

        cv::Mat tiledImg;
        if (isSVS(initialPaths[i]))
            osrFilenameToCVMat(this->initialPaths[i], tiledImg);
        else
            tiledImg = cv::imread(this->initialPaths[i]);

        // For each tile of the current image
        for (cv::Rect_<int64_t> tile : tiles[i]) {

#ifdef PROFILING2
            costs.emplace_back(this->cfunc->cost(tiledImg, tile));
            perims.emplace_back(2*tile.width + 2*tile.height);
#endif // #ifdef PROFILING2

#ifdef DEBUG
            // Print tile
            std::cout << "\ttile " << tile.x << ":" << tile.width
                << ", " << tile.y << ":" << tile.height << std::endl;

            // Adds tile rectangle region to tiled image
            cv::rectangle(tiledImg, 
                cv::Point(tile.x,tile.y), 
                cv::Point(tile.x+tile.width,
                          tile.y+tile.height),
                (0,0,0),3);
#endif // #ifdef DEBUG

        }

#ifdef DEBUG
        if (!tilingOnly) {
            std::string outname = "./tiled-" + this->initialPaths[i] + ".png";
            cv::imwrite(outname, tiledImg);
        }
#endif // #ifdef DEBUG

    }

#ifdef PROFILING2
    // Calculates stddev of tiles cost
    float mean = 0;
    for (int64_t c : costs)
        mean += c;
    mean /= costs.size();
    float var = 0;
    for (int64_t c : costs)
        var += pow(c-mean, 2);
    std::cout << std::fixed << "[PROFILING][AVERAGE] " << mean << std::endl;
    std::cout << std::fixed << "[PROFILING][STDDEV] " 
        << (sqrt(var/(costs.size()-1))) << std::endl;

    // Calculates the sum of perimeters
    int64_t sumPerim = 0;
    for (int64_t p : perims)
        sumPerim += p;
    std::cout << "[PROFILING][SUMOFPERIMS] " << sumPerim << std::endl;

#endif // #ifdef PROFILING2

#endif // #if defined(DEBUG) || defined(PROFILING2)

}

// DEPRECATED
// Performs the tiling using a previously tiled TRTC
void TiledRTCollection::tileImages(
    std::vector<std::list<cv::Rect_<int64_t>>> tiles) {

    // std::string cmd = "mkdir " + this->tilesPath + "/" + this->name;
    // const int dir_err = system(cmd.c_str());
    // if (dir_err == -1) {
    //     std::cout << "Error creating directory. " << __FILE__ << ":" 
    //         << __LINE__ << std::endl;
    //     exit(1);
    // }

    // // A tiling process can only occur once
    // if (this->tiled) {
    //     std::cout << "RT collection already tiled. Cannot re-tile it. " 
    //         << __FILE__ << ":" << __LINE__ << std::endl;
    //     cleanup(this->tilesPath + "/" + name);
    //     exit(-1);
    //     // return;
    // }

    // // The number of internal masks' paths and input tilings must match
    // if (tiles.size() != initialPaths.size()) {
    //     std::cout << "Internal masks' paths and input tilings size mismatch. "
    //         << "Expected "  << initialPaths.size() << " but got " << tiles.size()
    //         << ". " << __FILE__ << ":" << __LINE__ << std::endl;
    //     cleanup(this->tilesPath + "/" + name);
    //     exit(-1);
    //     // return;
    // }

    // // Iterate through all input images indices 
    // for (int i=0; i<tiles.size(); i++) {
    //     bool isSvs = isSVS(initialPaths[i]);

    //     // Open image for tiling
    //     openslide_t* osr;
    //     int32_t osrMaxLevel = 0; // svs standard: max level = 0
    //     cv::Mat mat;
    //     if (isSvs) {
    //         osr = openslide_open(initialPaths[i].c_str());
    //     } else {
    //         mat = cv::imread(initialPaths[i]);
    //     }

    //     // Go through all tiles
    //     int j = 0;
    //     for (cv::Rect_<int64_t> r : tiles[i]) {
    //         // Create a tile file
    //         cv::Mat tile;
    //         if (isSvs) {
    //             osrRegionToCVMat(osr, r, osrMaxLevel, tile);
    //         } else {
    //             tile = mat(r);
    //         }
    //         std::string path = this->tilesPath + name;
    //         path += "/i" + to_string(i) + "t" + to_string(j) + TILE_EXT;
    //         cv::imwrite(path, tile);

    //         // Create new RT tile from ROI r
    //         DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
    //         std::string drName = "t" + to_string(j);
    //         ddr2d->setName(drName);
    //         ddr2d->setId(refDDRName);
    //         ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
    //         ddr2d->setIsAppInput(true);
    //         ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
    //         ddr2d->setInputFileName(path);
    //         RegionTemplate* newRT = new RegionTemplate();
    //         newRT->insertDataRegion(ddr2d);
    //         newRT->setName(this->name);

    //         this->rts.push_back(
    //             std::pair<std::string, RegionTemplate*>(drName, newRT));

    //         j++;
    //     }

    //     // Close .svs file
    //     if (isSvs) {
    //         openslide_close(osr);
    //     }
    // }

    // this->tiled = true;
}

std::vector<std::list<cv::Rect_<int64_t>>> TiledRTCollection::getTiles() {
    if (!this->tiled) {
        std::cout << "Tiles not yet generated. " 
            << __FILE__ << ":" << __LINE__ << std::endl;
        // cleanup(this->tilesPath + "/" + name);
        exit(-1);
    }
    return tiles;
}

/*****************************************************************************/
/***************************** Helper functions ******************************/
/*****************************************************************************/

// DEPRECATED
void cleanup(std::string path) {
    // std::string cmd = "rm -rf " + path;
    // const int dir_err = system(cmd.c_str());
    // if (dir_err == -1) {
    //     std::cout << "Error cleaning up. " << __FILE__ 
    //         << ":" << __LINE__ << std::endl;
    //     exit(1);
    // }
}
