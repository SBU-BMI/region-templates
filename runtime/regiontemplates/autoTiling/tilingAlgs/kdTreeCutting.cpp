#include "kdTreeCutting.h"

// Splits an 'initial' tile in two equally sized tiles, according to the 
// input 'orient'ation.
void splitTileArea(rect_t initial, rect_t& newL, rect_t& newR, bool orient) {
    if (orient) {
        int64_t xlen = initial.xo - initial.xi;
        newL = {initial.xi, initial.yi, initial.xi+xlen/2, initial.yo};
        newR = {initial.xi+xlen/2+1, initial.yi, initial.xo, initial.yo};
    } else {
        int64_t ylen = initial.yo - initial.yi;
        newL = {initial.xi, initial.yi, initial.xo, initial.yi+ylen/2};
        newR = {initial.xi, initial.yi+ylen/2+1, initial.xo, initial.yo};
    }
}

// Splits an 'initial' tile in two tiles with the same cost, according
// to the input 'orient'ation.
void splitTileCost(const cv::Mat& img, rect_t initial, rect_t& newL, 
    rect_t& newR, bool orient) {

    int cost = cv::sum(img)[0]/2;
    splitTileLog(initial, img, cost, newL, newR, 0.2, orient?1:-1);
}

void kdTreeCutting(const cv::Mat& img, std::list<rect_t>& dense, 
    int nTiles, TilerAlg_t type) {

    // Calculates the number of cuts tree levels required
    int levels = ceil(log((float)nTiles/dense.size())/log(2));
    std::list<rect_t> result;

    for (rect_t initial : dense) {
        std::list<rect_t> *oldAreas, *newAreas, *tmp;
        oldAreas = new std::list<rect_t>();
        newAreas = new std::list<rect_t>();
        oldAreas->push_back(initial);
        bool orient = false;
        int curTiles = 0;
        for (int i=0; i<levels && curTiles<nTiles; i++) {
            for (std::list<rect_t>::iterator r=oldAreas->begin(); 
                r!=oldAreas->end(); r++) {

                rect_t newL, newR;
                if (type == KD_TREE_ALG_AREA)
                    splitTileArea(*r, newL, newR, orient);
                else if (type == KD_TREE_ALG_COST)
                    splitTileCost(img, *r, newL, newR, orient);
                else {
                    std::cout << "[kdTreeCutting] Bad kdTreeCutting"
                        << " alg type: " << type << std::endl;
                    exit(-1);
                }
                // avoids the creation of a null tile
                if (area(newL) > 0 && area(newR) > 0) { 
                    newAreas->push_back(newL);
                    newAreas->push_back(newR);
                    curTiles++;
                } else {
                    newAreas->push_back(*r);
                }

                if (curTiles >= nTiles) {
                    newAreas->insert(r++, oldAreas->begin(), oldAreas->end());
                    break;
                }
            }
            // Adds the remaining areas if curTiles reached nTiles
            orient = !orient;
            tmp = oldAreas;
            oldAreas = newAreas;
            newAreas = tmp;
            newAreas->clear();
        }
        result.insert(result.end(), oldAreas->begin(), oldAreas->end());
    }

    // Moves regions to the output list
    dense.clear();
    for (rect_t r : result) {
        dense.push_back(r);
    }
}
