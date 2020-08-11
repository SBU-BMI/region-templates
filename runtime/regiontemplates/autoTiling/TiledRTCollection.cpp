#include "TiledRTCollection.h"
#include "costFuncs/MultiObjCostFunction.h"
#include "costFuncs/ThresholdBGCostFunction.h"

/*****************************************************************************/
/****************************** Class methods ********************************/
/*****************************************************************************/

TiledRTCollection::TiledRTCollection(std::string name, std::string refDDRName,
                                     std::string tilesPath, int64_t borders,
                                     CostFunction* cfunc) {
    this->tiled = false;
    this->preTiled = false;
    this->drGen = false;
    this->name = name;
    this->refDDRName = refDDRName;
    this->tilesPath = tilesPath;
    this->borders = borders;
    this->cfunc = cfunc;
}

TiledRTCollection::~TiledRTCollection() {}

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
void TiledRTCollection::customTiling() {}

// Performs the autoTiler algorithm while updating the internal tiles
//   representation std::map<int, std::vector<cv::Rect_<int64_t>>>
void TiledRTCollection::tileImages(bool tilingOnly) {
    // A tiling process can only occur once
    if (this->tiled) {
        std::cout << "RT collection already tiled. Cannot re-tile it. "
                  << __FILE__ << ":" << __LINE__ << std::endl;
        exit(-1);
    }

    // Generate initial tile for each initial image if image
    // was not previously tiled.
    if (!this->preTiled) {
        for (std::string img : this->initialPaths) {
            // Open image
            openslide_t* osr;
            osr = openslide_open(img.c_str());

            // Gets dimensions of smallest slide image
            int osrMinLevel = openslide_get_level_count(osr) - 1;  // last lvl
            int64_t w, h;
            openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
            cv::Rect_<int64_t> roi(0, 0, w, h);

            // Close .svs file
            openslide_close(osr);

            // Adds single tile with full image
            std::list<cv::Rect_<int64_t>> begTiles;
            begTiles.push_back(roi);
            this->tiles[img] = begTiles;
        }
    }

    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    // tile containing the full image.
    customTiling();
    this->tiled = true;
}

void TiledRTCollection::generateDRs(bool tilingOnly) {
    if (!this->tiled) this->tileImages(tilingOnly);

    // Tiles' DRs generation can only occur once
    if (this->drGen) {
        std::cout << "RT collection already generated. Cannot re-tile it. "
                  << __FILE__ << ":" << __LINE__ << std::endl;
        exit(-1);
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
            if (isSVS(img)) {
                openslide_t* osr = openslide_open(img.c_str());
                int osrMinLevel =
                    openslide_get_level_count(osr) - 1;  // last level
                int64_t w = -1;
                int64_t h = -1;
                openslide_get_level0_dimensions(osr, &w, &h);
                std::cout << "[TiledRTCollection] Full image size: " << h << "x"
                          << w << std::endl;
                openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
                cv::Rect_<int64_t> roi(0, 0, w, h);
                osrRegionToCVMat(osr, roi, osrMinLevel, baseImg);
            } else
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
            int id = 0;

            // For each tile of the current image
            for (cv::Rect_<int64_t> tile : this->tiles[img]) {
                // Gets basic info from tile
                int64_t tileCost = this->cfunc->cost(baseImg, tile);
                costs.emplace_back(tileCost);
                perims.emplace_back(2 * tile.width + 2 * tile.height);

                // Print tile with readable unber format
                setlocale(LC_NUMERIC, "pt_BR.utf-8");
                char c_cost[50];
                sprintf(c_cost, "%'2ld", tileCost);
                // std::cout << "\ttile " << tile.y << ":" << tile.height
                //     << "\t" << tile.x << ":" << tile.width << "\tcost: "
                //     << c_cost << std::endl;
                std::cout << "\ttile a" << (tile.height * tile.width) << "\tc"
                          << c_cost << std::endl;

                // Adds tile rectangle region to tiled image
                cv::rectangle(
                    tiledImg, cv::Point(tile.x, tile.y),
                    cv::Point(tile.x + tile.width, tile.y + tile.height),
                    (255, 255, 255), 5);

                // Add cost to image as text
                cv::putText(tiledImg, c_cost,
                            cv::Point(tile.x + 10, tile.y + tile.height / 2),
                            cv::FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 7);
            }

#ifdef DEBUG
            // remove "/"
            int slLoc;
            std::string outname = img;
            while ((slLoc = outname.find("/")) != std::string::npos) {
                outname.replace(slLoc, 1, "");
            }
            outname = "./tiled-" + outname + ".png";
            cv::imwrite(outname, tiledImg);
#endif
        }
    }

    // Calculates stddev of tiles cost
    double mean = 0;
    for (double c : costs) mean += c;
    mean /= costs.size();
    double var = 0;
    for (int64_t c : costs) var += pow(c - mean, 2);
    double stddev = sqrt(var / (costs.size() - 1));

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
    for (int64_t p : perims) sumPerim += p;
    std::cout << "[PROFILING][SUMOFPERIMS] " << sumPerim << std::endl;

    // Converts tiles sizes to the large image, adds borders and
    // creates DRs
    if (!tilingOnly)
        for (std::string img : this->initialPaths) {
            // Open image
            openslide_t* osr;
            osr = openslide_open(img.c_str());

            // Gets info of largest image
            int64_t w, h;
            openslide_get_level0_dimensions(osr, &w, &h);
            std::cout << "[TiledRTCollection] Full image size: " << h << "x"
                      << w << std::endl;
            int64_t w0 = w;
            int64_t h0 = h;
            float ratiow = w;
            float ratioh = h;

            // Gets dimensions of smallest slide image
            int osrMinLevel = openslide_get_level_count(osr) - 1;  // last lvl
            openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
            cv::Rect_<int64_t> roi(0, 0, w, h);

            // Calculates the ratio between largest and smallest
            // images' dimensions for later conversion
            ratiow /= w;
            ratioh /= h;

            // Adds borders and creates DRs
            int drId = 0;
            std::list<cv::Rect_<int64_t>>::iterator tileIt =
                this->tiles[img].begin();

            // REMOVE AFTER SINGLE TILE TEST ---------
            //std::advance(tileIt, 33);
            //tileIt->x *= ratiow;
            //tileIt->width *= ratiow;
            //tileIt->y *= ratioh;
            //tileIt->height *= ratioh;
            //if (ceil(tileIt->x + tileIt->width) >= (int64_t)w0)
            //    tileIt->width = (int64_t)(w0 - tileIt->x);
            //if (ceil(tileIt->y + tileIt->height) >= (int64_t)h0)
            //    tileIt->height = (int64_t)(h0 - tileIt->y);
            //for (int i = 0; i < this->tiles[img].size(); i++) {
                // ---------
            for (; tileIt != this->tiles[img].end(); tileIt++) {
                // Converts the tile roi for the large image size
                tileIt->x *= ratiow;
                tileIt->width *= ratiow;
                tileIt->y *= ratioh;
                tileIt->height *= ratioh;
                if (ceil(tileIt->x + tileIt->width) >= (int64_t)w0)
                    tileIt->width = (int64_t)(w0 - tileIt->x);
                if (ceil(tileIt->y + tileIt->height) >= (int64_t)h0)
                     tileIt->height = (int64_t)(h0 - tileIt->y);

                // Adds borders
                tileIt->x -= this->borders;
                tileIt->x = std::max(0l, tileIt->x);
                tileIt->y -= this->borders;
                tileIt->y = std::max(0l, tileIt->y);
                tileIt->width += this->borders;
                tileIt->width = std::min(w0, tileIt->width);
                tileIt->height += this->borders;
                tileIt->height = std::min(h0, tileIt->height);

                // Creates the dr as a svs data region for
                // lazy read/write of input file
                DataRegion* dr = new DenseDataRegion2D();
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

    this->drGen = true;
}

void TiledRTCollection::setPreTiles(
    std::map<std::string, std::list<cv::Rect_<int64_t>>> tiles) {
    // Tiles' DRs generation can only occur once
    if (this->preTiled || this->tiled) {
        std::cout << "Can only set pre-tiles once and before tiling. "
                  << __FILE__ << ":" << __LINE__ << std::endl;
        exit(-1);
    }
    this->preTiled = true;
    this->tiles = tiles;
}

void TiledRTCollection::addTiles(
    std::map<std::string, std::list<cv::Rect_<int64_t>>> newTiles) {
    for (std::string img : this->initialPaths) {
        this->tiles[img].insert(this->tiles[img].end(), newTiles[img].begin(),
                                newTiles[img].end());
    }
}

void TiledRTCollection::addTargets(std::vector<Target_t> ts) {
    this->tileTarget.insert(this->tileTarget.end(), ts.begin(), ts.end());
}

std::vector<std::list<cv::Rect_<int64_t>>> TiledRTCollection::getTiles() {
    if (!this->tiled) {
        std::cout << "Tiles not yet generated. " << __FILE__ << ":" << __LINE__
                  << std::endl;
        exit(-1);
    }

    std::vector<std::list<cv::Rect_<int64_t>>> tiles;
    for (std::string img : this->initialPaths) {
        tiles.push_back(this->tiles[img]);
    }

    return tiles;
}
