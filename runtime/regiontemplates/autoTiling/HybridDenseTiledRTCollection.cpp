#include "HybridDenseTiledRTCollection.h"

HybridDenseTiledRTCollection::HybridDenseTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int64_t borders, 
    CostFunction* cfunc, BGMasker* bgm, TilerAlg_t tilingAlg, 
        int nCpuTiles, int nGpuTiles, float cpuPATS, float gpuPATS) 
        : TiledRTCollection(name, refDDRName, tilesPath, borders, cfunc) {

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

        this->tileMat(maskMat, this->tiles[img]);

        int drId = 0;
        for (cv::Rect_<int64_t> tile : this->tiles[img]) {
            this->tileTarget.push_back(drId<this->nGpuTiles ?
                ExecEngineConstants::GPU : ExecEngineConstants::CPU);
        }
    }
}

void HybridDenseTiledRTCollection::tileMat(cv::Mat& mat, std::list<cv::Rect_<int64_t>>& tiles) {
    // Generates Threshold mask
    cv::Mat thMask = bgm->bgMask(mat);
    thMask.convertTo(thMask, CV_8U);
    cv::threshold(thMask, thMask, 0, 255, cv::THRESH_BINARY);

    int w = mat.cols;
    int h = mat.rows;

    // Performs actual dense tiling
    switch (this->tilingAlg) {
        case FIXED_GRID_TILING: {
            std::cout << "[HDT][FIXED_GRID_TILING] Expected tiles: cpu " 
                << this->nCpuTiles << ", gpu " << this->nGpuTiles 
                << std::endl;
            std::cout << "[HDT][FIXED_GRID_TILING] Img size " 
                << h << "x" << w << std::endl;

            // Checks if no pre-tiling happened (i.e., only one dense tile)
            if (tiles.size() > 1) {
                std::cout << "[HDT][FIXED_GRID_TILING] Hybrid fixed grid " 
                    << " cannot be used with pre-tiling." << std::endl;
                exit(0);
            }

            // Percentage of gpu tile
            float f = 0.5;

            // Tile gpu subimage (top half)
            long gw = h>w ? w : w*f;
            long gmw = 0;
            long gh = h>w ? h*f : h;
            long gmh = 0;
            // std::cout << "[HDT][FIXED_GRID_TILING] gpu tiles:" 
            //     << std::endl;
            fixedGrid(this->nGpuTiles, gw, gh, gmw, gmh, tiles);

            // Tile cpu subimage (second half)
            long cw  = h>w ? w : w-w*f;
            long cmw = h>w ? 0 : w*f;
            long ch  = h>w ? h-h*f : h;
            long cmh = h>w ? h*f : 0;
            // std::cout << "[HDT][FIXED_GRID_TILING] cpu tiles:" 
            //     << std::endl;
            fixedGrid(this->nCpuTiles, cw, ch, cmw, cmh, tiles);

            break;
        }
        case LIST_ALG_EXPECT: {
            std::cout << "[HDT][LIST_ALG_EXPECT] Tiling for cpu="
                << this->nCpuTiles << ":" << this->cpuPATS << ", gpu="
                << this->nGpuTiles << ":" << this->gpuPATS << std::endl;
            
            // Converts the initial tiles list to rect_t
            std::list<rect_t> tmpTtiles;
            for (cv::Rect_<int64_t> r : tiles) {
                rect_t rt = {r.x, r.y, r.x+r.width-1, r.y+r.height-1};
                tmpTtiles.push_back(rt);
            }

            // Performs tiling
            listCutting(thMask, tmpTtiles, this->nCpuTiles, this->nGpuTiles, 
                this->cpuPATS, this->gpuPATS, this->cfunc);

            // Convert rect_t to cv::Rect_
            for (std::list<rect_t>::iterator r=tmpTtiles.begin(); 
                    r!=tmpTtiles.end(); r++) {
                tiles.push_back(cv::Rect_<int64_t>(
                    r->xi, r->yi, r->xo-r->xi, r->yo-r->yi));
            }
            break;
        }
        default:
            std::cout << "[HybridDenseTiledRTCollection] Invalid dense " 
                << "tiling algorithm." << std::endl;
            exit(-1);
    }
}
