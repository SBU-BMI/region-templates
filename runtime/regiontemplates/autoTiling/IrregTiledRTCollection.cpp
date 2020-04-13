#include "IrregTiledRTCollection.h"

IrregTiledRTCollection::IrregTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int border, 
    CostFunction* cfunc, BGMasker* bgm, TilerAlg_t tilingAlg, int nTiles) 
        : TiledRTCollection(name, refDDRName, tilesPath, cfunc) {

    this->border = border;
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
        // Create the list of tiles for the current image
        std::list<cv::Rect_<int64_t>> rois;

        // Open image for tiling
        // int64_t w0 = -1;
        // int64_t h0 = -1;
        int64_t w = -1;
        int64_t h = -1;
        openslide_t* osr;
        int32_t osrMinLevel = -1;
        // int32_t osrMaxLevel = 0; // svs standard: max level = 0
        // float ratiow;
        // float ratioh; 
        cv::Mat maskMat;

        // Opens svs input file
        osr = openslide_open(img.c_str());

        // // Gets info of largest image
        // openslide_get_level0_dimensions(osr, &w0, &h0);
        // ratiow = w0;
        // ratioh = h0;

        // Opens smallest image as a cv mat
        osrMinLevel = openslide_get_level_count(osr) - 1; // last level
        openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
        cv::Rect_<int64_t> roi(0, 0, w, h);
        osrRegionToCVMat(osr, roi, osrMinLevel, maskMat);

        // // Calculates the ratio between largest and smallest 
        // // images' dimensions for later conversion
        // ratiow /= w;
        // ratioh /= h;

        // Close .svs file
        openslide_close(osr);

        // Converts the initial tiles list to rect_t
        std::list<rect_t> curTiles;
        for (cv::Rect_<int64_t> r : this->tiles[img.c_str()]) {
            rect_t rt = {r.x, r.y, r.x+r.width-1, r.y+r.height-1};
            curTiles.push_back(rt);
        }
        // curTiles.push_back({0, 0, maskMat.cols, maskMat.rows});
        
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
// #ifdef DEBUG
//             cv::rectangle(maskMat, cv::Point(r->xi,r->yi), 
//                 cv::Point(r->xo,r->yo),(255,255,255),3);
// #endif

            // r->xi = std::max(r->xi-this->border, (int64_t)0);
            // r->xo = std::min(r->xo+this->border, (int64_t)maskMat.cols);
            // r->yi = std::max(r->yi-this->border, (int64_t)0);
            // r->yo = std::min(r->yo+this->border, (int64_t)maskMat.rows);

            // std::cout << r->xi << ":" << r->xo-r->xi+1 << ","
            //     << r->yi << ":" << r->yo-r->yi+1 << std::endl;

            tiles.push_back(cv::Rect_<int64_t>(
                r->xi, r->yi, r->xo-r->xi+1, r->yo-r->yi+1));
        }

        this->tiles[img.c_str()] = tiles;

// #ifdef DEBUG
//         cv::imwrite("./maskf.png", maskMat);
// #endif
        // std::cout << "ratioh " << ratioh << std::endl;
        // std::cout << "ratiow " << ratiow << std::endl;

        // std::cout << "w0 " << w0 << std::endl;
        // std::cout << "h0 " << h0 << std::endl;

        // // Actually tile the image given the list of ROIs
        // int drId=0;
        // for (cv::Rect_<int64_t> tile : tiles) {
        //     // Creates the tile name for the original svs file, if lazy
        //     std::string path = this->tilesPath;

        //     // Converts the tile roi for the bigger image
        //     tile.x *= ratiow;
        //     tile.width *= ratiow;
        //     tile.y *= ratioh;
        //     tile.height *= ratioh;

        //     // std::cout << tile.x << ":" << tile.width << ","
        //     //     << tile.y << ":" << tile.height << std::endl;

        //     // tile.x = std::max((int64_t)floor(ratiow*(tile.x-1)+1), 
        //     //     (int64_t)0);            
        //     if (ceil(tile.x+tile.width) >= (int64_t)w0)
        //         tile.width = (int64_t)(w0-tile.x);
        //     // tile.y = std::max((int64_t)floor(ratioh*tile.y), 
        //     //     (int64_t)0);
        //     if (ceil(tile.y+tile.height) >= (int64_t)h0)
        //         tile.height = (int64_t)(h0-tile.y);
        //     // else
        //     //     tile.height = (int64_t)ceil(ratioh*tile.height);

        //     // Creates the dr as a svs data region for
        //     // lazy read/write of input file
        //     DataRegion *dr = new DenseDataRegion2D();
        //     dr->setRoi(tile);
        //     dr->setSvs();

        //     // Create new RT tile from roi
        //     std::string drName = "t" + to_string(drId);
        //     dr->setName(drName);
        //     dr->setId(refDDRName);
        //     dr->setInputType(DataSourceType::FILE_SYSTEM);
        //     dr->setIsAppInput(true);
        //     dr->setOutputType(DataSourceType::FILE_SYSTEM);
        //     dr->setInputFileName(path);
        //     RegionTemplate* newRT = new RegionTemplate();
        //     newRT->insertDataRegion(dr);
        //     newRT->setName(name);

        //     // Add the tile and the RT to the internal containers
        //     rois.push_back(tile);
        //     this->rts.push_back(
        //         std::pair<std::string, RegionTemplate*>(drName, newRT));
        //     drId++;
        // }

        // // Add the current image tiles to the tiles vector
        // this->tiles.push_back(rois);
    }
}
