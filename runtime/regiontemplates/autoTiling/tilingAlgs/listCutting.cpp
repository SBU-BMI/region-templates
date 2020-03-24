#include "listCutting.h"

void listCutting(const cv::Mat& img, std::list<rect_t>& dense, 
    int nTiles, TilerAlg_t type, CostFunction* cfunc) {

    // Calculates the target average cost of a dense tile
    int avgCost = cv::sum(img)[0]/nTiles;

    // Create a multiset of tiles ordered by the cost function. This is to 
    // avoid re-sorting of the dense list whilst enabling O(1) access
    std::multiset<rect_t, rect_tCostFunct> sDense((rect_tCostFunct(img, cfunc)));
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
            splitTileLog(*dIt, img, cfunc, 
                cfunc->cost(img, dIt->yi, dIt->yo, dIt->xi, dIt->xo)/2, newt1, newt2);
        else if (type == LIST_ALG_EXPECT)
            splitTileLog(*dIt, img, cfunc, avgCost, newt1, newt2);
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

// GPU tiles come before cpu tiles
void listCutting(const cv::Mat& img, std::list<rect_t>& dense, int cpuCount, 
    int gpuCount, float cpuPats, float gpuPats, CostFunction* cfunc) {

    // Calculates the target average cost of a dense tile
    long imgCost = cfunc->cost(img);
    double totCost = cpuCount * cpuPats + gpuCount * gpuPats;
    double cpuCost = imgCost * cpuPats/totCost;
    double gpuCost = imgCost * gpuPats/totCost;

    // Create a multiset of tiles ordered by the cost function. This is to 
    // avoid re-sorting of the dense list whilst enabling O(1) access
    std::multiset<rect_t, rect_tCostFunct> sDense((rect_tCostFunct(img, cfunc)));
    for (rect_t r : dense) {
        sDense.insert(r);
    }
    dense.clear();

    setlocale(LC_NUMERIC, "pt_BR.utf-8");
    char cost[50];
    sprintf(cost, "%'2f", cfunc->cost(img));
    std::cout << "[listCutting] Image full cost: " << cost << std::endl;
    sprintf(cost, "%'2f", gpuCost);
    std::cout << "[listCutting] GPU tile expected cost: " << cost << std::endl;
    sprintf(cost, "%'2f", cpuCost);
    std::cout << "[listCutting] CPU tile expected cost: " << cost << std::endl;

    // Get gpu tiles
    for (int i=0; i<gpuCount; i++) {
        // Gets the first region (highest cost) 
        std::multiset<rect_t, rect_tCostFunct>::iterator dIt = sDense.begin();

        sprintf(cost, "%'2f", cfunc->cost(img, dIt->yi, dIt->yo, dIt->xi, dIt->xo));
        std::cout << "[listCutting] gpu init cost: " << cost << std::endl;

        // Splits tile with highest cost, generating a two new tiles, being 
        // one of them with close to avgCost cost.
        rect_t newt1, newt2;
        if ((dIt->xo-dIt->xi) == 1 || (dIt->yo-dIt->yi) == 1) {
            std::cout << "[listCutting] Tile too small to split."
                << std::endl;
            exit(-1);
        }
        splitTileLog(*dIt, img, cfunc, gpuCost, newt1, newt2);

        // Find which tile is the most expensive
        double c1 = cfunc->cost(img, newt1.yi, newt1.yo, newt1.xi, newt1.xo);
        double c2 = cfunc->cost(img, newt2.yi, newt2.yo, newt2.xi, newt2.xo);

        // Removes the first tile and insert the remaining large tile
        sDense.erase(dIt);
        if (c1 < c2) {
            sDense.insert(newt1);
            dense.push_back(newt2);
            sprintf(cost, "%'2f", c2);
            std::cout << "[listCutting] adding tile to gpu: " << cost << std::endl;
            std::cout << "\t" << newt2.yi << ":" << newt2.yo << "," 
                << newt2.xi << ":" << newt2.xo << std::endl;
        } else {
            sDense.insert(newt2);
            dense.push_back(newt1);
            sprintf(cost, "%'2f", c1);
            std::cout << "[listCutting] adding tile to gpu: " << cost << std::endl;
            std::cout << "\t" << newt1.yi << ":" << newt1.yo << "," 
                << newt1.xi << ":" << newt1.xo << std::endl;
        }
    }

    for (int i=1; i<cpuCount; i++) {
        // Gets the first region (highest cost) 
        std::multiset<rect_t, rect_tCostFunct>::iterator dIt = sDense.begin();

        sprintf(cost, "%'2f", cfunc->cost(img, dIt->yi, dIt->yo, dIt->xi, dIt->xo));
        std::cout << "[listCutting] cpu init cost: " << cost << std::endl;

        // Splits tile with highest cost, generating a two new tiles, being 
        // one of them with close to avgCost cost.
        rect_t newt1, newt2;
        if ((dIt->xo-dIt->xi) == 1 || (dIt->yo-dIt->yi) == 1) {
            std::cout << "[listCutting] Tile too small to split."
                << std::endl;
            exit(-1);
        }
        splitTileLog(*dIt, img, cfunc, cpuCost, newt1, newt2);

        // Find which tile is the most expensive
        double c1 = cfunc->cost(img, newt1.yi, newt1.yo, newt1.xi, newt1.xo);
        double c2 = cfunc->cost(img, newt2.yi, newt2.yo, newt2.xi, newt2.xo);

        // Removes the first tile and insert the remaining large tile
        sDense.erase(dIt);
        if (c1 > c2) {
            sDense.insert(newt1);
            dense.push_back(newt2);
            sprintf(cost, "%'2f", c2);
            std::cout << "[listCutting] adding tile to cpu: " << cost << std::endl;
            std::cout << "\t" << newt2.yi << ":" << newt2.yo << "," 
                << newt2.xi << ":" << newt2.xo << std::endl;
        } else {
            sDense.insert(newt2);
            dense.push_back(newt1);
            sprintf(cost, "%'2f", c1);
            std::cout << "[listCutting] adding tile to cpu: " << cost << std::endl;
            std::cout << "\t" << newt1.yi << ":" << newt1.yo << "," 
                << newt1.xi << ":" << newt1.xo << std::endl;
        }
    }

    // Moves regions to the output list
    for (rect_t r : sDense) {
        sprintf(cost, "%'2f", cfunc->cost(img, r.yi, r.yo, r.xi, r.xo));
        std::cout << "[listCutting] adding tile to cpu: " << cost <<std::endl;
        std::cout << "\t" << r.yi << ":" << r.yo << "," 
                << r.xi << ":" << r.xo << std::endl;
        dense.push_back(r);
    }
}
