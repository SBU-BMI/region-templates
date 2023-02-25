#include "HybridDenseTiledRTCollection.h"
#include "bgRemListCutting.h"
#include "fineBgRemoval.h"

HybridDenseTiledRTCollection::HybridDenseTiledRTCollection(
    std::string name, std::string refDDRName, std::string tilesPath,
    int64_t borders, CostFunction *cfunc, BGMasker *bgm, TilerAlg_t tilingAlg,
    int nCpuTiles, int nGpuTiles, float cpuPATS, float gpuPATS, bool fgBgRem)
    : TiledRTCollection(name, refDDRName, tilesPath, borders, cfunc) {
    this->bgm       = bgm;
    this->nCpuTiles = nCpuTiles;
    this->nGpuTiles = nGpuTiles;
    this->cpuPATS   = cpuPATS;
    this->gpuPATS   = gpuPATS;
    this->tilingAlg = tilingAlg;
    this->fgBgRem   = fgBgRem;
}

// May break when using more than one input image since DR id is unique
// only between input images.
void HybridDenseTiledRTCollection::customTiling() {
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

        this->tileMat(maskMat, this->tiles[img], this->bgTiles[img]);

        int drId = 0;
        for (cv::Rect_<int64_t> tile : this->tiles[img]) {
            // if (drId < this->nGpuTiles)
            //     std::cout << "[HDT]\tadding GPU tile.\n";
            // if (drId >= this->nGpuTiles)
            //     std::cout << "[HDT]\tadding CPU tile.\n";
            this->tileTarget.push_back(drId < this->nGpuTiles
                                           ? ExecEngineConstants::GPU
                                           : ExecEngineConstants::CPU);
            drId++;
        }
    }
}

void HybridDenseTiledRTCollection::tileMat(
    cv::Mat &mat, std::list<cv::Rect_<int64_t>> &tiles,
    std::list<cv::Rect_<int64_t>> &bgTiles) {
    int w = mat.cols;
    int h = mat.rows;

    // Performs actual dense tiling
    if (this->tilingAlg == LIST_ALG_EXPECT) {
        std::cout << "[HDT][LIST_ALG_EXPECT] Tiling ECL for cpu="
                  << this->nCpuTiles << ":" << this->cpuPATS
                  << ", gpu=" << this->nGpuTiles << ":" << this->gpuPATS
                  << std::endl;

        // Converts the initial tiles list to rect_t
        std::list<rect_t> curTiles;
        for (cv::Rect_<int64_t> r : tiles) {
            rect_t rt = {r.x, r.y, r.x + r.width, r.y + r.height};
            curTiles.push_back(rt);
        }

        // Performs fine-grain background removal
        std::list<rect_t> newDense;

        if (fgBgRem) {
            std::list<rect_t> bg;
            fineBgRemoval(bgm->bgMask(mat), curTiles, newDense, bg);

            // std::cout << "bg generated: " << bg.size() << "\n";

            // Convert BG partitions to Rect_ and set them
            bgTiles.clear();
            for (auto r = bg.begin(); r != bg.end(); r++) {
                bgTiles.push_back(cv::Rect_<int64_t>(
                    r->xi, r->yi, r->xo - r->xi + 1, r->yo - r->yi + 1));
            }

            curTiles = newDense;
        }

        // Performs tiling
        int dense = listCutting(mat, curTiles, this->nCpuTiles, this->nGpuTiles,
                                this->cpuPATS, this->gpuPATS, this->cfunc);

        // Correct gpuTiles count if there are many dense regions
        if (dense > 0)
            this->nGpuTiles =
                floor(this->gpuPATS / (this->gpuPATS + this->cpuPATS) * dense);

        tiles.clear();

        // Convert rect_t to cv::Rect_
        for (std::list<rect_t>::iterator r = curTiles.begin();
             r != curTiles.end(); r++) {
            tiles.push_back(
                cv::Rect_<int64_t>(r->xi, r->yi, r->xo - r->xi, r->yo - r->yi));
        }
    } else if (this->tilingAlg == HIER_FG_BR) {
        std::cout << "[HDT][HIER_FG_BR] Tiling ECL for cpu=" << this->nCpuTiles
                  << ":" << this->cpuPATS << ", gpu=" << this->nGpuTiles << ":"
                  << this->gpuPATS << std::endl;

        // Converts the initial tiles list to rect_t
        std::list<rect_t> curTiles;
        for (cv::Rect_<int64_t> r : tiles) {
            rect_t rt = {r.x, r.y, r.x + r.width, r.y + r.height};
            curTiles.push_back(rt);
        }

        // Performs tiling
        std::list<rect_t> bg;
        int               dense =
            bgRemListCutting(mat, curTiles, this->nCpuTiles, this->nGpuTiles,
                             this->cpuPATS, this->gpuPATS, this->cfunc, bg);

        // Correct gpuTiles count if there are many dense regions
        if (dense > 0)
            this->nGpuTiles =
                floor(this->gpuPATS / (this->gpuPATS + this->cpuPATS) * dense);

        tiles.clear();

        // Convert rect_t to cv::Rect_
        for (std::list<rect_t>::iterator r = curTiles.begin();
             r != curTiles.end(); r++) {
            tiles.push_back(
                cv::Rect_<int64_t>(r->xi, r->yi, r->xo - r->xi, r->yo - r->yi));
        }

    } else {
        // Checks if no pre-tiling happened (i.e., only one dense tile)
        if (tiles.size() > 1) {
            std::cout << "[HDT][tileMat] Only Hybrid ECL "
                      << " can be used with pre-tiling." << std::endl;
            exit(0);
        }
        tiles.clear();

        std::cout << "[HDT][tileMat] Tiling ECL for cpu=" << this->nCpuTiles
                  << ":" << this->cpuPATS << ", gpu=" << this->nGpuTiles << ":"
                  << this->gpuPATS << std::endl;

        // Percentage of gpu tile
        float f = (this->nGpuTiles * this->gpuPATS) /
                  ((this->nGpuTiles * this->gpuPATS) +
                   (this->nCpuTiles * this->cpuPATS));
        // f=0.5;

        // Create gpu tile (top half)
        long gw  = h > w ? w : w * f;
        long gmw = 0;
        long gh  = h > w ? h * f : h;
        long gmh = 0;

        // Create cpu tile (bottom half)
        long cw  = h > w ? w : w - w * f;
        long cmw = h > w ? 0 : w * f;
        long ch  = h > w ? h - h * f : h;
        long cmh = h > w ? h * f : 0;

        std::list<rect_t> curTiles;

        switch (this->tilingAlg) {
            case FIXED_GRID_TILING: {
                std::cout << "[HDT][FIXED_GRID_TILING] FG Expected tiles: cpu "
                          << this->nCpuTiles << ", gpu " << this->nGpuTiles
                          << std::endl;
                std::cout << "[HDT][FIXED_GRID_TILING] Img size " << h << "x"
                          << w << std::endl;

                // Tile gpu subimage (top half)
                // std::cout << "[HDT][FIXED_GRID_TILING] gpu tiles:" <<
                //              std::endl;
                fixedGrid(this->nGpuTiles, gw, gh, gmw, gmh, tiles);

                // Tile cpu subimage (second half)
                // std::cout << "[HDT][FIXED_GRID_TILING] cpu tiles:" <<
                //              std::endl;
                fixedGrid(this->nCpuTiles, cw, ch, cmw, cmh, tiles);

                break;
            }
            case KD_TREE_ALG_AREA:
            case KD_TREE_ALG_COST:
                std::cout
                    << "[HDT][FIXED_GRID_TILING] KD-Tree Expected tiles: cpu "
                    << this->nCpuTiles << ", gpu " << this->nGpuTiles
                    << std::endl;
                std::cout << "[HDT][FIXED_GRID_TILING] Img size " << h << "x"
                          << w << std::endl;

                // Tile gpu subimage (top half)
                curTiles.push_back({gmw, gmh, gmw + gw - 1, gmh + gh - 1});
                kdTreeCutting(mat, curTiles, this->nGpuTiles, this->tilingAlg,
                              this->cfunc);

                // Add gpu tiles to current result
                for (std::list<rect_t>::iterator r = curTiles.begin();
                     r != curTiles.end(); r++) {
                    tiles.push_back(cv::Rect_<int64_t>(
                        r->xi, r->yi, r->xo - r->xi, r->yo - r->yi));
                }
                curTiles.clear();

                // Tile cpu subimage (second half)
                curTiles.push_back({cmw, cmh, cmw + cw - 1, cmh + ch - 1});
                kdTreeCutting(mat, curTiles, this->nCpuTiles, this->tilingAlg,
                              this->cfunc);

                // Add cpu tiles to current result
                for (std::list<rect_t>::iterator r = curTiles.begin();
                     r != curTiles.end(); r++) {
                    tiles.push_back(cv::Rect_<int64_t>(
                        r->xi, r->yi, r->xo - r->xi, r->yo - r->yi));
                }
                break;

            case HBAL_TRIE_QUAD_TREE_ALG:
                std::cout
                    << "[HDT][FIXED_GRID_TILING] Quad-Tree Expected tiles: cpu "
                    << this->nCpuTiles << ", gpu " << this->nGpuTiles
                    << std::endl;
                std::cout << "[HDT][FIXED_GRID_TILING] Img size " << h << "x"
                          << w << std::endl;

                // Tile gpu subimage (top half)
                curTiles.push_back({gmw, gmh, gmw + gw - 1, gmh + gh - 1});
                heightBalancedTrieQuadTreeCutting(mat, curTiles,
                                                  this->nGpuTiles);

                // Add gpu tiles to current result
                for (std::list<rect_t>::iterator r = curTiles.begin();
                     r != curTiles.end(); r++) {
                    tiles.push_back(cv::Rect_<int64_t>(
                        r->xi, r->yi, r->xo - r->xi, r->yo - r->yi));
                }
                curTiles.clear();

                // Tile cpu subimage (second half)
                curTiles.push_back({cmw, cmh, cmw + cw - 1, cmh + ch - 1});
                heightBalancedTrieQuadTreeCutting(mat, curTiles,
                                                  this->nCpuTiles);

                // Add cpu tiles to current result
                for (std::list<rect_t>::iterator r = curTiles.begin();
                     r != curTiles.end(); r++) {
                    tiles.push_back(cv::Rect_<int64_t>(
                        r->xi, r->yi, r->xo - r->xi, r->yo - r->yi));
                }
                break;

            case CBAL_TRIE_QUAD_TREE_ALG:
            case CBAL_POINT_QUAD_TREE_ALG:
                std::cout << "[HDT][CBAL_POINT_QUAD_TREE_ALG] Quad-Tree "
                             "Expected tiles: cpu "
                          << this->nCpuTiles << ", gpu " << this->nGpuTiles
                          << std::endl;
                std::cout << "[HDT][CBAL_POINT_QUAD_TREE_ALG] Img size " << h
                          << "x" << w << std::endl;

                // Tile gpu subimage (top half)
                curTiles.push_back({gmw, gmh, gmw + gw - 1, gmh + gh - 1});
                costBalancedQuadTreeCutting(mat, curTiles, this->nGpuTiles,
                                            this->tilingAlg, this->cfunc);

                // Add gpu tiles to current result
                for (std::list<rect_t>::iterator r = curTiles.begin();
                     r != curTiles.end(); r++) {
                    tiles.push_back(cv::Rect_<int64_t>(
                        r->xi, r->yi, r->xo - r->xi, r->yo - r->yi));
                }
                curTiles.clear();

                // Tile cpu subimage (second half)
                curTiles.push_back({cmw, cmh, cmw + cw - 1, cmh + ch - 1});
                costBalancedQuadTreeCutting(mat, curTiles, this->nCpuTiles,
                                            this->tilingAlg, this->cfunc);

                // Add cpu tiles to current result
                for (std::list<rect_t>::iterator r = curTiles.begin();
                     r != curTiles.end(); r++) {
                    tiles.push_back(cv::Rect_<int64_t>(
                        r->xi, r->yi, r->xo - r->xi, r->yo - r->yi));
                }
                break;

            default:
                std::cout << "[HDT] Invalid dense "
                          << "tiling algorithm." << std::endl;
                exit(-1);
        }

        std::cout << "[HDT] Tiles: " << tiles.size() << std::endl;
    }
}
