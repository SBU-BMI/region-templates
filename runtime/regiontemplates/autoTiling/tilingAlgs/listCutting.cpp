#include "listCutting.h"

void listCutting(const cv::Mat& img, std::list<rect_t>& dense, 
    int nTiles, TilerAlg_t type) {

    // Calculates the target average cost of a dense tile
    int avgCost = cv::sum(img)[0]/nTiles;

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

        // Splits tile with highest cost, generating a two new tiles, being 
        // one of them with close to avgCost cost.
        rect_t newt1, newt2;
        if ((dIt->xo-dIt->xi) == 1 || (dIt->yo-dIt->yi) == 1) {
            std::cout << "[listCutting] Tile too small to split."
                << std::endl;
            exit(-1);
        }
        if (type == LIST_ALG_HALF)
            splitTileLog(*dIt, img, cost(img, *dIt)/2, newt1, newt2);
        else if (type == LIST_ALG_EXPECT)
            splitTileLog(*dIt, img, avgCost, newt1, newt2);
        else {
            std::cout << "[listCutting] Bad listCutting alg type: "
                << type << std::endl;
            exit(-1);
        }

        // Removes the first tile and insert the two sub-tiles created from it
        sDense.erase(dIt);
        sDense.insert(newt1);
        sDense.insert(newt2);
    }

    // Moves regions to the output list
    dense.clear();
    for (rect_t r : sDense)
        dense.push_back(r);
}
