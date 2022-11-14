#include "bgRemListCutting.h"
#include "Util.h"
#include "denseFromBG.h"
#include "tilingUtil.h"

#include <list>
#include <set>
#include <vector>

void bgRemListCutting(const cv::Mat &img, std::list<rect_t> &dense, int nTiles,
                      CostFunction *cfunc, std::list<rect_t> &bgPartitions) {
    long t1 = Util::ClockGetTime();
    // Create a multiset of tiles ordered by the cost function. This is to
    // avoid re-sorting of the dense list whilst enabling O(1) access
    std::multiset<rect_t, rect_tCostFunct> sDense(
        (rect_tCostFunct(img, cfunc)));
    for (rect_t r : dense) {
        sDense.insert(r);
    }

    long t2 = Util::ClockGetTime();
    // Get all multiples of the number of partitions
    std::list<int> multiples = getMultiples(nTiles);

    long tBTS = 0;
    long t3   = Util::ClockGetTime();
    // Perform hierarchical partitioning for each multiple of nTiles
    int expctParts = 1;
    for (int mult : multiples) {
        expctParts *= mult;
        std::cout << "[bgRemListCutting] hierarchical for " << expctParts
                  << " of multiple " << mult << "\n";

        // Calculates the target average expected cost of a dense tile
        // Cost function should not evaluate area
        long avgCost = 0;
        for (rect_t r : sDense) {
            avgCost += cfunc->cost(img, r.yi, r.yo, r.xi, r.xo);
        }
        avgCost /= expctParts;

        int initialTiles = sDense.size();

        setlocale(LC_NUMERIC, "pt_BR.utf-8");
        char ccost[50];
        sprintf(ccost, "%'2f", cfunc->cost(img));

        // Keeps breaking tiles until current expctParts goal is reached
        while (expctParts > sDense.size()) {
            // std::cout << "[bgRemListCutting] " << sDense.size() << " of "
            //           << expctParts << " done\n";

            // Gets the first region (highest cost)
            std::multiset<rect_t, rect_tCostFunct>::iterator dIt =
                sDense.begin();

            // Splits tile with highest cost, generating a two new tiles, being
            // one of them with close to avgCost cost.
            rect_t newt1, newt2;

            // Commented code should work for 1 pixel lines which
            // can be divided in half
            if ((dIt->xo - dIt->xi) == 1 || (dIt->yo - dIt->yi) == 1) {
                // if ((dIt->xo - dIt->xi) == 1 && (dIt->yo - dIt->yi) == 1) {
                std::cout << "[listCutting] Tile too small to split."
                          << std::endl;
                exit(-1);
            } else {
                // Partitions the current top tile in two
                long t11 = Util::ClockGetTime();
                bgRmSplitTileLog(*dIt, img, cfunc, avgCost, newt1, newt2,
                                 bgPartitions);
                long t12 = Util::ClockGetTime();
                tBTS += t12 - t11;
            }

            // Removes the first tile and insert the two
            // sub-tiles created from it
            sDense.erase(dIt);
            sDense.insert(newt1);
            sDense.insert(newt2);
        }
    }

    long t4 = Util::ClockGetTime();
    // Moves regions to the output list
    dense.clear();
    for (rect_t r : sDense) {
        dense.push_back(r);
    }
    long t5 = Util::ClockGetTime();

    std::cout << "[TILING][bgRemListCutting] main_loop: " << (t4 - t3) << "\n";
    std::cout << "[TILING][bgRemListCutting] bts_time: " << tBTS << "\n";
}
