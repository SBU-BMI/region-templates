#include "IrregTiledRTCollection.h"

IrregTiledRTCollection::IrregTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int64_t borders, 
    CostFunction* cfunc, BGMasker* bgm, TilerAlg_t tilingAlg, int nTiles) 
        : TiledRTCollection(name, refDDRName, tilesPath, borders, cfunc) {
            
    this->bgm = bgm;
    this->nTiles = nTiles;
    this->tilingAlg = tilingAlg;
}

// May break when using more than one input image since DR id is unique
// only between input images.
void IrregTiledRTCollection::customTiling() {
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

        // Converts the initial tiles list to rect_t
        std::list<rect_t> curTiles;
        for (cv::Rect_<int64_t> r : this->tiles[img.c_str()]) {
            rect_t rt = {r.x, r.y, r.x+r.width-1, r.y+r.height-1};
            curTiles.push_back(rt);
        }
        
        // Performs actual dense tiling
        switch (this->tilingAlg) {
            case LIST_ALG_HALF:
            case LIST_ALG_EXPECT: {
                listCutting(maskMat, curTiles, this->nTiles, 
                    this->tilingAlg, this->cfunc);
                break;
            }
            case KD_TREE_ALG_AREA:
            case KD_TREE_ALG_COST: {
                kdTreeCutting(maskMat, curTiles, this->nTiles, 
                    this->tilingAlg, this->cfunc);
                break;
            }
            case HBAL_TRIE_QUAD_TREE_ALG: {
                heightBalancedTrieQuadTreeCutting(maskMat, 
                    curTiles, this->nTiles);
                break;
            }
            case CBAL_TRIE_QUAD_TREE_ALG:
            case CBAL_POINT_QUAD_TREE_ALG: {
                costBalancedQuadTreeCutting(maskMat, curTiles, 
                    this->nTiles, this->tilingAlg, this->cfunc);
                break;
            }
        }

        // Convert rect_t to cv::Rect_ and add borders
        std::list<cv::Rect_<int64_t> > tiles;
        for (std::list<rect_t>::iterator r=curTiles.begin(); 
                r!=curTiles.end(); r++) {
            tiles.push_back(cv::Rect_<int64_t>(
                r->xi, r->yi, r->xo-r->xi+1, r->yo-r->yi+1));
        }

        this->tiles[img.c_str()] = tiles;
    }
}
