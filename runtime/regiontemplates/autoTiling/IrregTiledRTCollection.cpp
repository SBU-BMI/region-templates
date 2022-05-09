#include "IrregTiledRTCollection.h"
#include "bgRemListCutting.h"
#include "fineBgRemoval.h"

IrregTiledRTCollection::IrregTiledRTCollection(
    std::string name, std::string refDDRName, std::string tilesPath,
    int64_t borders, CostFunction *cfunc, BGMasker *bgm, TilerAlg_t tilingAlg,
    int nTiles, bool fgBgRem)
    : TiledRTCollection(name, refDDRName, tilesPath, borders, cfunc) {
    this->bgm       = bgm;
    this->nTiles    = nTiles;
    this->tilingAlg = tilingAlg;
    this->fgBgRem   = fgBgRem;
}

// May break when using more than one input image since DR id is unique
// only between input images.
void IrregTiledRTCollection::customTiling() {
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
    }
}

void IrregTiledRTCollection::tileMat(cv::Mat                       &mat,
                                     std::list<cv::Rect_<int64_t>> &tiles,
                                     std::list<cv::Rect_<int64_t>> &bgTiles) {
    // Converts the initial tiles list to rect_t
    std::list<rect_t> curTiles;
    for (cv::Rect_<int64_t> r : tiles) {
        rect_t rt = {r.x, r.y, r.x + r.width - 1, r.y + r.height - 1};
        curTiles.push_back(rt);
    }

    // Performs actual dense tiling
    switch (this->tilingAlg) {
        case LIST_ALG_HALF:
        case LIST_ALG_EXPECT: {
            // Performs fine-grain background removal
            std::list<rect_t> dense;

            // if (fgBgRem) {

            //     std::list<rect_t> bg;
            //     fineBgRemoval(bgm->bgMask(mat), curTiles, dense, bg);

            //     // std::cout << "bg generated: " << bg.size() << "\n";

            //     // Convert BG partitions to Rect_ and set them
            //     bgTiles.clear();
            //     // for (auto r = bg.begin(); r != bg.end(); r++) {
            //     //     bgTiles.push_back(cv::Rect_<int64_t>(
            //     //         r->xi, r->yi, r->xo - r->xi + 1, r->yo - r->yi +
            //     1));
            //     // }

            //     curTiles = dense;
            // }

            listCutting(mat, curTiles, this->nTiles, this->tilingAlg,
                        this->cfunc);
            break;
        }
        case KD_TREE_ALG_AREA:
        case KD_TREE_ALG_COST: {
            kdTreeCutting(mat, curTiles, this->nTiles, this->tilingAlg,
                          this->cfunc);
            break;
        }
        case HBAL_TRIE_QUAD_TREE_ALG: {
            heightBalancedTrieQuadTreeCutting(mat, curTiles, this->nTiles);
            break;
        }
        case CBAL_TRIE_QUAD_TREE_ALG:
        case CBAL_POINT_QUAD_TREE_ALG: {
            costBalancedQuadTreeCutting(mat, curTiles, this->nTiles,
                                        this->tilingAlg, this->cfunc);
            break;
        }
        case HIER_FG_BR:
            bgTiles.clear();
            std::list<rect_t> bg;
            bgRemListCutting(mat, curTiles, this->nTiles, this->cfunc, bg);
            // for (auto r = bg.begin(); r != bg.end(); r++) {
            //     bgTiles.push_back(cv::Rect_<int64_t>(
            //         r->xi, r->yi, r->xo - r->xi + 1, r->yo - r->yi + 1));
            // }
    }

    // Convert rect_t to cv::Rect_
    tiles.clear();
    for (std::list<rect_t>::iterator r = curTiles.begin(); r != curTiles.end();
         r++) {
        tiles.push_back(cv::Rect_<int64_t>(r->xi, r->yi, r->xo - r->xi + 1,
                                           r->yo - r->yi + 1));
    }
}
