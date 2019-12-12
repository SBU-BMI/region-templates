#include "quadTreeCutting.h"

void trieSplit4(const rect_t& r, std::list<rect_t>& newTs) {
    int64_t w2 = (r.xo-r.xi)/2;
    int64_t h2 = (r.yo-r.yi)/2;
    newTs.push_back({r.xi, r.yi, r.xi+w2, r.yi+h2});
    newTs.push_back({r.xi+w2+1, r.yi, r.xo, r.yi+h2});
    newTs.push_back({r.xi, r.yi+h2+1, r.xi+w2, r.yo});
    newTs.push_back({r.xi+w2+1, r.yi+h2+1, r.xo, r.yo});
}

void costBasedSplit4(const rect_t& r, const cv::Mat& img, 
    std::list<rect_t>& newTs, CostFunction* cfunc) {

    int64_t tileCost = cost(img, r);

    // Performs the log split for both vertical and horizontal orientations
    rect_t newt1h, newt2h;
    splitTileLog(r, img, cfunc, tileCost/2, newt1h, newt2h, 0.2, -1);
    rect_t newt1v, newt2v;
    splitTileLog(r, img, cfunc, tileCost/2, newt1v, newt2v, 0.2, 1);

    // Creates the tiles by intersecting both splits
    newTs.push_back({r.xi, r.yi, newt1h.xo, newt1v.yo});
    newTs.push_back({newt2h.xi, r.yi, r.xo, newt1v.yo});
    newTs.push_back({r.xi, newt2v.yi, newt1h.xo, r.yo});
    newTs.push_back({newt2h.xi, newt2v.yi, r.xo, r.yo});
}

// A balanced quad-trie structure is generated on which the image is
// split in a 4-ary tree fashion, having the restriction that the 
// largest difference between the level of two leaf nodes is 1.
// If the number of tiles is reached before the trie is filled
// to the last level, the first tiles of the trie are chosen.
void heightBalancedTrieQuadTreeCutting(const cv::Mat& img, 
    std::list<rect_t>& dense, int nTiles) {

    // Calculates the number of cuts tree levels required
    int levels = ceil(log(nTiles/dense.size())/log(4));
    std::list<rect_t> result;

    for (rect_t initial : dense) {
        std::list<rect_t> *oldAreas, *newAreas, *tmp;
        oldAreas = new std::list<rect_t>();
        newAreas = new std::list<rect_t>();
        oldAreas->push_back(initial);
        int curTiles = 0;

        // Go through the number of expected levels
        for (int i=0; i<levels && curTiles<nTiles; i++) {
            // Split each tile of a given level
            for (std::list<rect_t>::iterator r=oldAreas->begin(); 
                r!=oldAreas->end(); r++) {

                // Avoid the creation of a null tile (i.e., zero area tile)
                if (r->xo-r->xi <= 1 || r->yo-r->yi <= 1) {
                    dense.push_back(*r);
                    continue;
                }

                // Splits the current tile and add them to the next level
                std::list<rect_t> newTs;
                trieSplit4(*r, newTs);
                r=oldAreas->erase(r);
                newAreas->insert(newAreas->end(), newTs.begin(), newTs.end());
                curTiles += 3; // 1 original removed and 4 smaller added

                // Adds the remaining tiles to the next level since the 
                // number of necessary tiles was reached
                if (curTiles >= nTiles) {
                    newAreas->insert(newAreas->begin(), r++, oldAreas->end());
                    break;
                }
            }
            // Adds the remaining areas if curTiles reached nTiles
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

// An unbalanced quad-trie structure is used to generate the tiles by splitting
// the tile with the highest cost. The partition can be regular (i.e., one tile
// is split into 4 equal-sized sub-tiles) or cost sensitive (i.e., the tile is 
// split in the attempt to balance the cost of the new sub-tiles).
// At least nTiles are always generated.
void costBalancedQuadTreeCutting(const cv::Mat& img, 
    std::list<rect_t>& dense, int nTiles, TilerAlg_t type, CostFunction* cfunc) {

    // Create a multiset of tiles ordered by the cost function. This is to 
    // avoid re-sorting of the dense list whilst enabling O(1) access
    std::multiset<rect_t, rect_tCostFunct> sDense((rect_tCostFunct(img)));
    for (rect_t r : dense) {
        sDense.insert(r);
    }

    // Keeps breaking tiles until nTiles goal is reached
    while (sDense.size() < nTiles) {
        // Gets the first region (highest cost) 
        std::multiset<rect_t, rect_tCostFunct>::iterator dIt = sDense.begin();

        // Checks whether the split of tile dIt could generate an empty tile
        if ((dIt->xo-dIt->xi) <= 1 || (dIt->yo-dIt->yi) <= 1) {
            // std::cout << "[quadTreeCutting] Tile too small to split."
            //     << std::endl;

            // Removes the tile from the search list and add it to the result 
            // list, thus avoiding it being split
            dense.push_back(*dIt);
            sDense.erase(dIt);
            continue;
        }

        // Splits tile with highest cost, generating a 4 new tiles
        std::list<rect_t> newTs;
        if (type == CBAL_TRIE_QUAD_TREE_ALG)
            trieSplit4(*dIt, newTs);
        else if (type == CBAL_POINT_QUAD_TREE_ALG)
            costBasedSplit4(*dIt, img, newTs, cfunc);
        else {
            std::cout << "[costBalancedQuadTreeCutting] Bad "
                << "costBalancedQuadTreeCutting alg type: " 
                << type << std::endl;
            exit(-1);
        }

        // Removes the first tile and insert the new sub-tiles created from it
        sDense.erase(dIt);
        sDense.insert(newTs.begin(), newTs.end());
    }

    // Moves regions to the output list
    dense.clear();
    for (rect_t r : sDense)
        dense.push_back(r);
}
