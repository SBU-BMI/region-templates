#include "quadTreeCutting.h"

void trieSplit4(const rect_t& r, std::list<rect_t>& newTs) {
    int64_t w2 = (r.xo-r.xi)/2;
    int64_t h2 = (r.yo-r.yi)/2;
    newTs.push_back({r.xi, r.yi, r.xi+w2, r.yi+h2});
    newTs.push_back({r.xi+w2+1, r.yi, r.xo, r.yi+h2});
    newTs.push_back({r.xi, r.yi+h2+1, r.xi+w2, r.yo});
    newTs.push_back({r.xi+w2+1, r.yi+h2+1, r.xo, r.yo});
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
// the tile with the highest cost. The partition is regular (i.e., one tile is
// split into 4 equal-sized sub-tiles) and generates at least nTiles.
void costBalancedTrieQuadTreeCutting(const cv::Mat& img, 
    std::list<rect_t>& dense, int nTiles) {

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
        trieSplit4(*dIt, newTs);

        // Removes the first tile and insert the new sub-tiles created from it
        sDense.erase(dIt);
        sDense.insert(newTs.begin(), newTs.end());
    }

    // Moves regions to the output list
    dense.clear();
    for (rect_t r : sDense)
        dense.push_back(r);
}
