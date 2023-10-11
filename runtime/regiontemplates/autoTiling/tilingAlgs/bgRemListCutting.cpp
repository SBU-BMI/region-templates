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
    // multiples = {2, 3, 5, 7};
    multiples = {7, 5, 3, 2};
    std::cout << "SUM ";
    for (int m : multiples)
        std::cout << m << ", ";
    std::cout << "\n";

    long tBTS = 0;
    long t3 = Util::ClockGetTime();
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
        std::cout << "=============== total img cost: " << ccost << "\n";

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

    // Resolve the 1 tile case
    if (sDense.size() == 1) {
        rect_t oldTile = *sDense.begin();
        std::list<rect_t> bg;
        sDense.clear();
        sDense.insert(simpleRemoveBg(img, oldTile, cfunc, bg));
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

int bgRemListCutting(const cv::Mat &img, std::list<rect_t> &dense, int cpuCount,
                     int gpuCount, float cpuPats, float gpuPats,
                     CostFunction *cfunc, std::list<rect_t> &bgPartitions) {
    // Checks if "dense" already have enough tiles
    // If so there is nothing to be done
    if (dense.size() >= cpuCount + gpuCount) {
        std::cout << "[bgRemListCutting] No tiling required: needed "
                  << (cpuCount + gpuCount) << " but already have "
                  << dense.size() << std::endl;
        return dense.size();
    }

    // Calculates the target average cost of a dense tile
    long long imgCost = 0;
    for (rect_t r : dense) {
        imgCost += cfunc->cost(img, r.yi, r.yo, r.xi, r.xo);
    }

    if (gpuCount == 0)
        gpuPats = 0;
    if (cpuCount == 0)
        cpuPats = 0;

    double totCost = cpuCount * cpuPats + gpuCount * gpuPats;
    double gpuCost = imgCost * gpuPats / totCost;
    double cpuCost = imgCost * cpuPats / totCost;

    // std::cout << "totCost(" << totCost << ") = cpuCount(" << cpuCount
    //           << ") * cpuPats(" << cpuPats << ") + GpuCount(" << gpuCount
    //           << ") * GpuPats(" << gpuPats << ")" << std::endl;

    // std::cout << "GpuCost(" << gpuCost << ") = imgCost(" << imgCost
    //           << ") * GpuPats(" << gpuPats << ") / totCost(" << totCost <<
    //           ")"
    //           << std::endl;

    // std::cout << "cpuCost(" << cpuCost << ") = imgCost(" << imgCost
    //           << ") * cpuPats(" << cpuPats << ") / totCost(" << totCost <<
    //           ")"
    //           << std::endl;

    int initialTiles = dense.size();

    // Create a multiset of tiles ordered by the cost function. This is to
    // avoid re-sorting of the dense list whilst enabling O(1) access
    std::multiset<rect_t, rect_tCostFunct> sDense(
        (rect_tCostFunct(img, cfunc)));
    for (rect_t r : dense) {
        sDense.insert(r);
    }
    dense.clear();

    setlocale(LC_NUMERIC, "pt_BR.utf-8");
    char ccost[50];
    // sprintf(ccost, "%'2f", cfunc->cost(img));
    // std::cout << "[listCutting] Image full cost: " << ccost << std::endl;
    // sprintf(ccost, "%'2f", gpuCost);
    // std::cout << "[listCutting] GPU tile expected cost: " << ccost <<
    // std::endl; sprintf(ccost, "%'2f", cpuCost); std::cout <<
    // "[listCutting]CPU tile expected cost: " << ccost << std::endl;

    // // Generate tiles for each device 1 GPU and n GPUs
    // int nGpuTiles = gpuCount/cpuCount;
    // while (sDense.size() < nGpuTiles+1) {
    // }

    // Get gpu tiles
    for (int i = 0; i < (gpuCount + cpuCount - initialTiles); i++) {
        // Gets the first region (highest cost)
        std::multiset<rect_t, rect_tCostFunct>::iterator dIt = sDense.begin();

        // sprintf(ccost, "%'2f", cfunc->cost(img, dIt->yi, dIt->yo, dIt->xi,
        // dIt->xo)); std::cout << "[listCutting] init cost: " << ccost <<
        // std::endl;

        // Splits tile with highest cost, generating a two new tiles, being
        // one of them with close to avgCost cost.
        rect_t newt1, newt2;

        double cost = i < gpuCount ? gpuCost : cpuCost;
        if ((dIt->xo - dIt->xi) == 1 && (dIt->yo - dIt->yi) == 1) {
            std::cout << "[bgRemListCutting] Tile too small to split."
                      << std::endl;
            exit(-1);
        } else if ((dIt->xo - dIt->xi) == 1) {
            int64_t pivot = (dIt->yo - dIt->yi) / 2 + dIt->yi;
            newt1 = {dIt->xi, dIt->yi, dIt->xo, pivot};
            newt2 = {dIt->xi, pivot + 1, dIt->xo, dIt->yo};
        } else if ((dIt->yo - dIt->yi) == 1) {
            int64_t pivot = (dIt->xo - dIt->xi) / 2 + dIt->xi;
            newt1 = {dIt->xi, dIt->yi, pivot, dIt->yo};
            newt2 = {pivot + 1, dIt->yi, dIt->xo, dIt->yo};
        } else {
            // splitTileLog(*dIt, img, cfunc, cost, newt1, newt2);
            bgRmSplitTileLog(*dIt, img, cfunc, cost, newt1, newt2,
                             bgPartitions);
        }

        // Find which tile is the most expensive
        double c1 = cfunc->cost(img, newt1.yi, newt1.yo, newt1.xi, newt1.xo);
        double c2 = cfunc->cost(img, newt2.yi, newt2.yo, newt2.xi, newt2.xo);

        // Removes the first tile and insert the remaining large tile
        sDense.erase(dIt);
        // std::cout << "[bgRemListCutting] =============== dist c1: " <<
        // abs(cost - c1) << std::endl; std::cout << "[bgRemListCutting]
        // =============== dist c2: " << abs(cost - c2) << std::endl;
        if (std::abs(cost - c1) > std::abs(cost - c2)) {
            sDense.insert(newt1);   // adding c1 to more tiling
            dense.push_back(newt2); // adding c2 to final
            sprintf(ccost, "%'2f", c2);
            // std::cout << "[bgRemListCutting] adding tile2: " << ccost <<
            // std::endl; std::cout << "\t" << newt2.yi << ":" << newt2.yo <<
            // "," << newt2.xi
            //           << ":" << newt2.xo << std::endl;
        } else {
            sDense.insert(newt2);
            dense.push_back(newt1); // adding c1 to final
            sprintf(ccost, "%'2f", c1);
            // std::cout << "[bgRemListCutting] adding tile1: " << ccost <<
            // std::endl; std::cout << "\t" << newt1.yi << ":" << newt1.yo <<
            // "," << newt1.xi
            //           << ":" << newt1.xo << std::endl;
        }
    }

    // Moves regions to the output list
    for (rect_t r : sDense) {
        char cost[90];
        sprintf(cost, "%'2f", cfunc->cost(img, r.yi, r.yo, r.xi, r.xo));
        std::cout << "[bgRemListCutting] adding tile to cpu: " << cost
                  << std::endl;
        std::cout << "\t" << r.yi << ":" << r.yo << "," << r.xi << ":" << r.xo
                  << std::endl;
        dense.push_back(r);
    }

    return 0;
}