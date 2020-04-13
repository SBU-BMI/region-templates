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

    // // Go through all images
    // for (int i=0; i<this->initialPaths.size(); i++) {
        
    //     cv::Mat tile;
    //     openslide_t* osr;

    //     // Checks file extension
    //     if (!isSVS(this->initialPaths[i])) {
    //         tile = cv::imread(this->initialPaths[i]);
    //     } else {
    //         // Open svs image for tiling
    //         int64_t w = -1;
    //         int64_t h = -1;
    //         int32_t osrMaxLevel = 0; // svs standard: max level = 0

    //         osr = openslide_open(this->initialPaths[i].c_str());

    //         // // get all levels info
    //         // int levels = openslide_get_level_count(osr);
    //         // std::cout << "=============levels " << levels << std::endl;
    //         // for (int i=0; i<levels; i++) {
    //         //     openslide_get_level_dimensions(osr, i, &w, &h);
    //         //     std::cout << "level " << i << " " << w 
    //         //               << "x" << h << std::endl;
    //         // }

    //         // Create the tile mat
    //         openslide_get_level0_dimensions(osr, &w, &h);
    //         cv::Rect_<int64_t> roi(0, 0, w, h);
    //         osrRegionToCVMat(osr, roi, osrMaxLevel, tile);
    //     }

    //     // Create the full roi and add it to the roi's vector
    //     cv::Rect_<int64_t> roi(0, 0, tile.cols, tile.rows);
    //     cv::Rect_<int64_t> rois[] = {roi};
    //     this->tiles.push_back(std::list<cv::Rect_<int64_t>>(rois, 
    //         rois + sizeof(rois)/sizeof(cv::Rect_<int64_t>)));

    //     // Create new RT tile from roi
    //     DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
    //     ddr2d->setName(drName);
    //     ddr2d->setId(this->refDDRName);
    //     ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
    //     ddr2d->setIsAppInput(true);
    //     ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
    //     ddr2d->setInputFileName(this->initialPaths[i]);
        
    //     // Sets svs ddr parameters and closes svs image
    //     if (isSVS(this->initialPaths[i])) {
    //         ddr2d->setSvs();
    //         ddr2d->setRoi(roi);
    //         openslide_close(osr);
    //     }

    //     RegionTemplate* newRT = new RegionTemplate();
    //     newRT->setName(this->name);
    //     newRT->insertDataRegion(ddr2d);

    //     this->rts.push_back(
    //         std::pair<std::string, RegionTemplate*>(drName, newRT));
    // }
}

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

    // Generate initial tile for each initial image if image 
    // was not previously tiled.
    if (!this->preTiled) {
        for (std::string img : this->initialPaths) {
            // Open image
            openslide_t* osr;
            osr = openslide_open(img.c_str());

            // Gets info of largest image
            int64_t w, h;
            openslide_get_level0_dimensions(osr, &w, &h);
            w0s[img] = w;
            h0s[img] = h;
            ratiows[img] = w;
            ratiohs[img] = h;

            // Gets dimensions of smallest slide image
            int osrMinLevel = openslide_get_level_count(osr) - 1; // last lvl
            openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
            cv::Rect_<int64_t> roi(0, 0, w, h);

            // Calculates the ratio between largest and smallest 
            // images' dimensions for later conversion
            ratiows[img] /= w;
            ratiohs[img] /= h;

            // Close .svs file
            openslide_close(osr);

            // Adds single tile with full image
            std::list<cv::Rect_<int64_t> > begTiles;
            begTiles.push_back(roi);
            this->tiles[img] = begTiles;
        }
    }

    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    // tile containing the full image.
    customTiling();
    this->tiled = true;

    // Converts tiles sizes to the large image, adds borders and
    // creates DRs
    for (std::string img : this->initialPaths) {
        // Adds borders and creates DRs
        int drId=0;
        // for (cv::Rect_<int64_t> tile : this->tiles[img]) {
        std::list<cv::Rect_<int64_t>>::iterator tileIt 
            = this->tiles[img].begin();
        for (; tileIt!=this->tiles[img].end(); tileIt++) {
            // Converts the tile roi for the large image size
            tileIt->x *= ratiows[img];
            tileIt->width *= ratiows[img];
            tileIt->y *= ratiohs[img];
            tileIt->height *= ratiohs[img];
            if (ceil(tileIt->x+tileIt->width) >= (int64_t)w0s[img])
                tileIt->width = (int64_t)(w0s[img]-tileIt->x);
            if (ceil(tileIt->y+tileIt->height) >= (int64_t)h0s[img])
                tileIt->height = (int64_t)(h0s[img]-tileIt->y);

            // Adds borders
            // TODO

            // Creates the dr as a svs data region for
            // lazy read/write of input file
            DataRegion *dr = new DenseDataRegion2D();
            dr->setRoi(*tileIt);
            dr->setSvs();

            // Create new RT tile from roi
            std::string drName = "t" + to_string(drId);
            dr->setName(drName);
            dr->setId(this->refDDRName);
            dr->setInputType(DataSourceType::FILE_SYSTEM);
            dr->setIsAppInput(true);
            dr->setOutputType(DataSourceType::FILE_SYSTEM);
            dr->setInputFileName(this->tilesPath);
            RegionTemplate* newRT = new RegionTemplate();
            newRT->insertDataRegion(dr);
            newRT->setName(this->name);

            // Add the tile and the RT to the internal containers
            this->rts.push_back(
                std::pair<std::string, RegionTemplate*>(drName, newRT));
            drId++;
        }
    }

    // Creates a list of costs for each tile for each image
    std::list<int64_t> costs;
    // Creates a list of perimeters for each tile for each image
    std::list<int64_t> perims;
#define DEBUG
    // create tiles image
    if (tilingOnly) {
        for (std::string img : this->initialPaths) {

            cv::Mat baseImg;
            if (isSVS(img))
                osrFilenameToCVMat(img, baseImg);
            else
                baseImg = cv::imread(img);
            
            setlocale(LC_NUMERIC, "pt_BR.utf-8");
            char cost[50];
            sprintf(cost, "%'2f", this->cfunc->cost(baseImg));
            std::cout << "[TiledRTCollection] image " << img 
                << " size: " << baseImg.rows << "x" << baseImg.cols 
                << " cost=" << cost << std::endl;

            // get cost image
            cv::Mat tiledImg;
            tiledImg = this->cfunc->costImg(baseImg);
            cv::cvtColor(tiledImg, tiledImg, cv::COLOR_GRAY2RGB);

            // tiles files ids
            int id=0;

            // For each tile of the current image
            for (cv::Rect_<int64_t> tile : this->tiles[img]) {
                // Gets basic info from tile
                int64_t tileCost = this->cfunc->cost(baseImg, tile);
                costs.emplace_back(tileCost);
                perims.emplace_back(2*tile.width + 2*tile.height);

                // Print tile with readable unber format
                setlocale(LC_NUMERIC, "pt_BR.utf-8");
                char c_cost[50];
                sprintf(c_cost, "%'2ld", tileCost);
                std::cout << "\ttile " << tile.y << ":" << tile.height
                    << "\t" << tile.x << ":" << tile.width << "\tcost: " 
                    << c_cost << std::endl;


                // Adds tile rectangle region to tiled image
                cv::rectangle(tiledImg, 
                    cv::Point(tile.x,tile.y), 
                    cv::Point(tile.x+tile.width,
                              tile.y+tile.height),
                    (255,255,255),5);

                // Add cost to image as text
                cv::putText(tiledImg, 
                    c_cost,
                    cv::Point(tile.x+10, tile.y+tile.height/2),
                    cv::FONT_HERSHEY_SIMPLEX, 3, (255,255,255), 7);
            }

            #ifdef DEBUG
            // remove "/"
            int slLoc;
            std::string outname = img;
            while ((slLoc=outname.find("/")) != std::string::npos) {
                outname.replace(slLoc, 1, "");
            }
            outname = "./tiled-" + outname + ".png";
            cv::imwrite(outname, tiledImg);
            #endif
        }
    }

    // Calculates stddev of tiles cost
    double mean = 0;
    for (double c : costs)
        mean += c;
    mean /= costs.size();
    double var = 0;
    for (int64_t c : costs)
        var += pow(c-mean, 2);
    double stddev = sqrt(var/(costs.size()-1));
    
    // Make the results readable for humans...
    setlocale(LC_NUMERIC, "pt_BR.utf-8");
    char c_mean[150];
    char c_stddev[150];
    sprintf(c_mean, "%'2lf", mean);
    sprintf(c_stddev, "%'2lf", stddev);
    std::cout << "[PROFILING][AVERAGE] " << c_mean << std::endl;
    std::cout << "[PROFILING][STDDEV] " << c_stddev << std::endl;

    // Calculates the sum of perimeters
    int64_t sumPerim = 0;
    for (int64_t p : perims)
        sumPerim += p;
    std::cout << "[PROFILING][SUMOFPERIMS] " << sumPerim << std::endl;

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
    //         << "Expected "  << initialPaths.size() 
    //         << " but got " << tiles.size()
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

    std::vector<std::list<cv::Rect_<int64_t>>> tiles;
    for (std::string img : this->initialPaths) {
        tiles.push_back(this->tiles[img]);
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
