#include "HybridDenseTiledRTCollection.h"

HybridDenseTiledRTCollection::HybridDenseTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int border, 
    CostFunction* cfunc, BGMasker* bgm, TilerAlg_t tilingAlg, 
        int nCpuTiles, int nGpuTiles, float cpuPATS, float gpuPATS) 
        : TiledRTCollection(name, refDDRName, tilesPath, cfunc) {

    this->border = border;
    this->bgm = bgm;
    this->nCpuTiles = nCpuTiles;
    this->nGpuTiles = nGpuTiles;
    this->cpuPATS = cpuPATS;
    this->gpuPATS = gpuPATS;
    this->tilingAlg = tilingAlg;
}

// May break when using more than one input image since DR id is unique
// only between input images.
void HybridDenseTiledRTCollection::customTiling() {
    std::string drName;
    // Go through all images
    for (int i=0; i<this->initialPaths.size(); i++) {
        // Create the list of tiles for the current image
        std::list<cv::Rect_<int64_t>> rois;

        // Open image for tiling
        int64_t w = -1;
        int64_t h = -1;
        openslide_t* osr;
        int32_t osrMinLevel = -1;
        int32_t osrMaxLevel = 0; // svs standard: max level = 0
        float ratiow;
        float ratioh; 
        cv::Mat maskMat;

        // Opens svs input file
        osr = openslide_open(this->initialPaths[i].c_str());

        // Gets info of largest image
        openslide_get_level0_dimensions(osr, &w, &h);
        ratiow = w;
        ratioh = h;

        // Opens smallest image as a cv mat
        osrMinLevel = openslide_get_level_count(osr) - 1; // last level
        openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
        cv::Rect_<int64_t> roi(0, 0, w, h);
        osrRegionToCVMat(osr, roi, osrMinLevel, maskMat);

        // Calculates the ratio between largest and smallest 
        // images' dimensions for later conversion
        ratiow /= w;
        ratioh /= h;

        // Output tiles
        std::list<cv::Rect_<int64_t> > finalTiles;

        // Creates a list of a single full-image tile for dense tiling
        std::list<rect_t> tiles;
        cv::Mat thMask = bgm->bgMask(maskMat);
        tiles.push_back({0, 0, maskMat.cols, maskMat.rows});
        
        // Ensure that thMask is a binary mat
        thMask.convertTo(thMask, CV_8U);
        cv::threshold(thMask, thMask, 0, 255, cv::THRESH_BINARY);

        // Performs actual dense tiling
        switch (this->tilingAlg) {
            case FIXED_GRID_TILING: {
                std::cout << "[HDT][FIXED_GRID_TILING] Expected tiles: cpu " 
                    << this->nCpuTiles << ", gpu " << this->nGpuTiles 
                    << std::endl;
                std::cout << "[HDT][FIXED_GRID_TILING] Img size " 
                    << h << "x" << w << std::endl;

                // Percentage of gpu tile
                float f = 0.6;

                // Tile gpu subimage (top half)
                long gw = h>w ? w : w*f;
                long gmw = 0;
                long gh = h>w ? h*f : h;
                long gmh = 0;
                std::cout << "[HDT][FIXED_GRID_TILING] gpu tiles:" << std::endl;
                fixedGrid(this->nGpuTiles, gw, gh, gmw, gmh, finalTiles);

                // Tile cpu subimage (second half)
                long cw  = h>w ? w : w-w*f;
                long cmw = h>w ? 0 : w*f;
                long ch  = h>w ? h-h*f : h;
                long cmh = h>w ? h*f : 0;
                std::cout << "[HDT][FIXED_GRID_TILING] cpu tiles:" << std::endl;
                fixedGrid(this->nCpuTiles, cw, ch, cmw, cmh, finalTiles);

                break;
            }
            case LIST_ALG_EXPECT: {
                std::cout << "[HDT][LIST_ALG_EXPECT] Tiling for cpu="
                    << this->nCpuTiles << ":" << this->cpuPATS << ", gpu="
                    << this->nGpuTiles << ":" << this->gpuPATS << std::endl;
                listCutting(thMask, tiles, this->nCpuTiles, this->nGpuTiles, 
                    this->cpuPATS, this->gpuPATS, this->cfunc);

                // Convert rect_t to cv::Rect_
                for (std::list<rect_t>::iterator r=tiles.begin(); 
                        r!=tiles.end(); r++) {
                    finalTiles.push_back(cv::Rect_<int64_t>(
                        r->xi, r->yi, r->xo-r->xi, r->yo-r->yi));
                }
                break;
            }
            default:
                std::cout << "[HybridDenseTiledRTCollection] Invalid dense " 
                    << "tiling algorithm." << std::endl;
                exit(-1);
        }

        // // Convert rect_t to cv::Rect_ and add borders
        // std::list<cv::Rect_<int64_t> > tiles;
        // for (std::list<rect_t>::iterator r=tiles.begin(); 
        //         r!=tiles.end(); r++) {

        //     r->xi = std::max(r->xi-this->border, (int64_t)0);
        //     r->xo = std::min(r->xo+this->border, (int64_t)thMask.cols);
        //     r->yi = std::max(r->yi-this->border, (int64_t)0);
        //     r->yo = std::min(r->yo+this->border, (int64_t)thMask.rows);

        //     tiles.push_back(cv::Rect_<int64_t>(
        //         r->xi, r->yi, r->xo-r->xi, r->yo-r->yi));
            
        //     #ifdef DEBUG
        //     cv::rectangle(thMask, cv::Point(r->xi,r->yi), 
        //         cv::Point(r->xo,r->yo),(255,255,255),3);
        //     #endif
        // }

#ifdef DEBUG
        cv::imwrite("./maskf.png", thMask);
#endif

        // Actually tile the image given the list of ROIs
        int drId=0;
        for (cv::Rect_<int64_t> tile : finalTiles) {
            // Creates the tile name for the original svs file, if lazy
            std::string path = this->tilesPath;

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
            dr->setId(refDDRName);
            dr->setInputType(DataSourceType::FILE_SYSTEM);
            dr->setIsAppInput(true);
            dr->setOutputType(DataSourceType::FILE_SYSTEM);
            dr->setInputFileName(path);
            RegionTemplate* newRT = new RegionTemplate();
            newRT->insertDataRegion(dr);
            newRT->setName(name);

            // Add the tile and the RT to the internal containers
            rois.push_back(tile);
            this->rts.push_back(
                std::pair<std::string, RegionTemplate*>(drName, newRT));
            this->tileTarget.push_back(drId<this->nGpuTiles ?
                ExecEngineConstants::GPU : ExecEngineConstants::CPU);
            drId++;
        }

        // Close .svs file
        openslide_close(osr);

        // Add the current image tiles to the tiles vector
        this->tiles.push_back(rois);
    }
}
