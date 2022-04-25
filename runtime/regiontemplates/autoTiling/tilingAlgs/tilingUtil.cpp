#include "tilingUtil.h"

std::list<rect_t> toMyRectT(std::list<cv::Rect_<int64_t>> from) {
    std::list<rect_t> to;
    for (auto r : from) {
        to.emplace_back(r.x, r.y, r.x + r.width - 1, r.y + r.height - 1);
    }
    return to;
}

std::string rect_t::toStr() {
    std::string ret = "[";
    ret += std::to_string(this->xi);
    ret += ", ";
    ret += std::to_string(this->xo);
    ret += "], [";
    ret += std::to_string(this->yi);
    ret += ", ";
    ret += std::to_string(this->yo);
    ret += "]";

    return ret;
}

/*****************************************************************************/
/**                            I/O and Profiling                            **/
/*****************************************************************************/

// prints the std dev of a tile inside an image. Used for profiling
void stddev(std::list<rect_t> rs, const cv::Mat &img, std::string name,
            CostFunction *cfunc) {
    float mean = 0;
    for (rect_t r : rs)
        mean += cfunc->cost(img, r.yi, r.yo, r.xi, r.xo);
    mean /= rs.size();

    float var = 0;
    for (rect_t r : rs)
        var += pow(cfunc->cost(img, r.yi, r.yo, r.xi, r.xo) - mean, 2);

    std::cout << "[PROFILING][STDDEV][" << name << "]"
              << (sqrt(var / (rs.size() - 1))) << std::endl;
}

// Prints a rect_t without the carriage return
void printRect(rect_t r) {
    std::cout << r.xi << ":" << r.xo << "," << r.yi << ":" << r.yo;
}

// // Returns the sum of all pixels' values of the roi 'r' of an input image
// inline int pxSum(const cv::Mat& img, const cv::Rect_<int64_t>& r) {
//     return cv::sum(img(cv::Range(r.y, r.y+r.height),
//                        cv::Range(r.x, r.x+r.width)))[0];
// }

/*****************************************************************************/
/**                           Log Split Algorithm                           **/
/*****************************************************************************/

// Performs a binary sweep search across the input tile image, searching for
// a split on which the goal cost is found within an acc margin of error.
// At least one of the new tiles must have the goal cost. Both a vertical
// and a horizontal sweep are performed, being returned the pair of regions
// with the smallest difference between areas.
// Orient: 0 = both, -1 = horizontal only, +1 = vetical only
void splitTileLog(const rect_t &r, const cv::Mat &img, CostFunction *cfunc,
                  double expCost, rect_t &newt1, rect_t &newt2, float acc,
                  int orient) {

    // Gets upper and lower bounds for the expected cost
    long upperCost = expCost + expCost * acc;
    long lowerCost = expCost - expCost * acc;

    ///////////////////////////////////////////////////////////////////////////
    // Horizontal sweeping
    long pivotxLen = (r.xo - r.xi) / 2;
    long pivotx    = r.xi + pivotxLen;
    long cost1h    = cfunc->cost(img, r.yi, r.yo, r.xi, pivotx);
    long cost2h    = cfunc->cost(img, r.yi, r.yo, pivotx + 1, r.xo);
    long areah;

    // std::cout << "lowerCost " << lowerCost << std::endl;
    // std::cout << "upperCost " << upperCost << std::endl;

    if (orient <= 0) {

        // Finds through binary search the best horizontal pivot
        while (!between(cost1h, lowerCost, upperCost) &&
               !between(cost2h, lowerCost, upperCost) && pivotxLen > 0) {

            // std::cout << "cost1h " << cost1h << std::endl;
            // std::cout << "cost2h " << cost2h << std::endl;

            // If the expected cost was not achieved, split the pivotLen
            // and try again, moving the pivot closer to the tile with
            // the greatest cost.
            pivotxLen /= 2;
            if (cost1h > cost2h && expCost > cost2h && expCost < cost1h)
                pivotx -= pivotxLen;
            else
                pivotx += pivotxLen;

            // Avoid zero area regions
            if (pivotx - r.xi < 1 || r.xo - pivotx - 1 < 1)
                break;

            cost1h = cfunc->cost(img, r.yi, r.yo, r.xi, pivotx);
            cost2h = cfunc->cost(img, r.yi, r.yo, pivotx + 1, r.xo);
        }

        // Calculate difference between areas of new tiles 1 and 2
        areah = abs((r.yo - r.yi) * (r.xo + r.xi + 2 * pivotx));
    }

    ///////////////////////////////////////////////////////////////////////////
    // Vertical sweeping
    long pivotyLen = (r.yo - r.yi) / 2;
    long pivoty    = r.yi + pivotyLen;
    long cost1v    = cfunc->cost(img, r.yi, pivoty, r.xi, r.xo);
    long cost2v    = cfunc->cost(img, pivoty + 1, r.yo, r.xi, r.xo);
    long areav;
    if (orient >= 0) {

        // Finds through binary search the best vertical pivot
        while (!between(cost1v, lowerCost, upperCost) &&
               !between(cost2v, lowerCost, upperCost) && pivotyLen > 0) {

            // std::cout << "cost1v " << cost1v << std::endl;
            // std::cout << "cost2v " << cost2v << std::endl;

            // If the expected cost was not achieved, split the pivotLen
            // and try again, moving the pivot closer to the tile with
            // the greatest cost.
            pivotyLen /= 2;
            if (cost1v > cost2v && expCost > cost2v && expCost < cost1v)
                pivoty -= pivotyLen;
            else
                pivoty += pivotyLen;

            // Avoid zero area regions
            if (pivoty - r.yi < 1 || r.yo - pivoty - 1 < 1)
                break;

            cost1v = cfunc->cost(img, r.yi, pivoty, r.xi, r.xo);
            cost2v =
                cfunc->cost(img, std::max(0l, pivoty + 1), r.yo, r.xi, r.xo);
        }

        // Calculate difference between areas of new tiles 1 and 2
        areav = abs((r.xo - r.xi) * (r.yo + r.yi + 2 * pivoty));
    }

    ///////////////////////////////////////////////////////////////////////////
    // Assigns the regions coordinates
    newt1.xi = r.xi; // newt1 is left and newt2 is right
    newt1.yi = r.yi;
    newt2.xo = r.xo; // newt1 is left and newt2 is right
    newt2.yo = r.yo;

    // Checks whether vertical or horizontal sweep results were better
    if (orient < 0 || (orient == 0 && areah < areav)) { // horizontal
        newt1.xo = pivotx;
        newt2.xi = pivotx + 1;
        newt1.yo = r.yo;
        newt2.yi = r.yi;
    } else { // vertical
        newt1.xo = r.xo;
        newt2.xi = r.xi;
        newt1.yo = pivoty;
        newt2.yi = pivoty + 1;
    }
}

// Returns the main dense partition by value and the list of BG tiles by ref
rect_t simpleRemoveBg(const cv::Mat &img, rect_t tile, CostFunction *cfunc,
                      std::list<rect_t> &bgTiles) {

    // Find bounding box for dense area
    cv::Mat stats, centroids;
    cv::Mat mask = cfunc->costImg(img, tile.yi, tile.yo, tile.xi, tile.xo);
    cv::connectedComponentsWithStats(mask, mask, stats, centroids);

    // Get number of initial dense regions
    double maxLabel;
    cv::minMaxLoc(mask, NULL, &maxLabel);

    // generate the list of dense areas
    std::list<rect_t> denseCand;
    float             f       = 0.01;
    int               minArea = mask.cols * mask.rows * f;
    long              maxArea = 0;
    rect_t            curDenseTile;
    for (int i = 1; i <= maxLabel; i++) { // i=1 ignores background
        // std::cout << "area: "
        //    << stats.at<int>(i, cv::CC_STAT_AREA) << std::endl;
        // Ignores small areas
        if (stats.at<int>(i, cv::CC_STAT_AREA) > 500) {
            int xi = stats.at<int>(i, cv::CC_STAT_LEFT);
            int yi = stats.at<int>(i, cv::CC_STAT_TOP);
            int xw = stats.at<int>(i, cv::CC_STAT_WIDTH);
            int yh = stats.at<int>(i, cv::CC_STAT_HEIGHT);
            if (xw > 1 && yh > 1 && yh * xw > minArea) {
                rect_t rr = {.xi = xi, .yi = yi, .xo = xi + xw, .yo = yi + yh};
                denseCand.push_back(rr);

                if (yh * xw > maxArea) {
                    maxArea      = yh * xw;
                    curDenseTile = rr;
                }
            }
        }
    }

    // if (denseCand.size() > 1) {
    //     std::cout << "[simpleRemoveBg] GOT MORE THAN 1 DENSE
    //     REGION========\n"; for (rect_t r : denseCand) {
    //         std::cout << r.toStr() << "\n";
    //         cv::rectangle(mask, cv::Point(r.xi, r.yi), cv::Point(r.xo, r.yo),
    //                       (255, 255, 255), 5);
    //     }
    //     cv::imwrite("tmp.png", mask);
    //     exit(0);
    // }

    // Keep looking for new BG partitions
    long bgMinAreaThresh = 0.1 * (tile.yo - tile.yi) * (tile.xo - tile.xi);
    long xiLim           = tile.xi;
    long xoLim           = tile.xo;
    long yiLim           = tile.yi;
    long yoLim           = tile.yo;
    long xiLimBest       = tile.xi;
    long xoLimBest       = tile.xo;
    long yiLimBest       = tile.yi;
    long yoLimBest       = tile.yo;
    long bestArea;

    // curDenseTile coordinates are offsets to tile
    // Convert them to absolute coordinates
    curDenseTile.xi += tile.xi;
    curDenseTile.xo += tile.xi;
    curDenseTile.yi += tile.yi;
    curDenseTile.yo += tile.yi;

    // std::cout << xiLim << "\n";
    // std::cout << xoLim << "\n";
    // std::cout << yiLim << "\n";
    // std::cout << yoLim << "\n";
    // std::cout << curDenseTile.xi << "\n";
    // std::cout << curDenseTile.xo << "\n";
    // std::cout << curDenseTile.yi << "\n";
    // std::cout << curDenseTile.yo << "\n";
    // exit(0);
    while (true) {
        bestArea = 0;
        rect_t bestBg;

        // Test upper tile
        rect_t upperBg = {
            .xi = xiLim, .yi = yiLim, .xo = xoLim, .yo = curDenseTile.yi - 1};
        if (upperBg.size() > bestArea && upperBg.size() > bgMinAreaThresh) {
            bestArea  = upperBg.size();
            bestBg    = upperBg;
            xiLimBest = xiLim;
            xoLimBest = xoLim;
            yiLimBest = curDenseTile.yi;
            yoLimBest = yoLim;
            // std::cout << "up\n";
        }

        // Test lower tile
        rect_t lowerBg = {
            .xi = xiLim, .yi = curDenseTile.yo + 1, .xo = xoLim, .yo = yoLim};
        if (lowerBg.size() > bestArea && lowerBg.size() > bgMinAreaThresh) {
            bestArea  = lowerBg.size();
            bestBg    = lowerBg;
            xiLimBest = xiLim;
            xoLimBest = xoLim;
            yiLimBest = yiLim;
            yoLimBest = curDenseTile.yo;
            // std::cout << "down\n";
        }

        // Test left tile
        rect_t leftBg = {
            .xi = xiLim, .yi = yiLim, .xo = curDenseTile.xi - 1, .yo = yoLim};
        if (leftBg.size() > bestArea && leftBg.size() > bgMinAreaThresh) {
            bestArea  = leftBg.size();
            bestBg    = leftBg;
            xiLimBest = curDenseTile.xi;
            xoLimBest = xoLim;
            yiLimBest = yiLim;
            yoLimBest = yoLim;
            // std::cout << "left\n";
        }

        // Test right tile
        rect_t rightBg = {
            .xi = curDenseTile.xo + 1, .yi = yiLim, .xo = xoLim, .yo = yoLim};
        if (rightBg.size() > bestArea && rightBg.size() > bgMinAreaThresh) {
            bestArea  = rightBg.size();
            bestBg    = rightBg;
            xiLimBest = xiLim;
            xoLimBest = curDenseTile.xo;
            yiLimBest = yiLim;
            yoLimBest = yoLim;
            // std::cout << "right\n";
        }

        if (bestArea > bgMinAreaThresh) {
            bgTiles.push_back(bestBg);
            std::cout << curDenseTile.toStr() << "\n";
            // std::cout << "[simpleRemoveBg] added BG tile " << bestBg.toStr()
            //           << "\n";
            // std::cout << "[simpleRemoveBg] bestArea " << bestArea << "\n";
            // std::cout << "[simpleRemoveBg] bgMinAreaThresh " <<
            // bgMinAreaThresh
            //           << "\n";
            xiLim = xiLimBest;
            xoLim = xoLimBest;
            yiLim = yiLimBest;
            yoLim = yoLimBest;
            // exit(0);
        } else {
            // Reset dense tile to full tile if no BG is found
            curDenseTile = {.xi = xiLimBest,
                            .yi = yiLimBest,
                            .xo = xoLimBest,
                            .yo = yoLimBest};
            break;
        }
    }

    return curDenseTile;
}

void costWithBgRm(const cv::Mat &img, CostFunction *cfunc, rect_t &tile1,
                  rect_t &tile2, std::list<rect_t> &remainingBg, long &cost1,
                  long &cost2) {
    // Remove background from both tiles
    // std::cout << "=========r1\n";
    tile1 = simpleRemoveBg(img, tile1, cfunc, remainingBg);
    // std::cout << "=========r2\n";
    tile2 = simpleRemoveBg(img, tile2, cfunc, remainingBg);

    // Calculate cost
    cost1 = cfunc->cost(img, tile1.yi, tile1.yo, tile1.xi, tile1.xo);
    cost2 = cfunc->cost(img, tile2.yi, tile2.yo, tile2.xi, tile2.xo);
}

// Background removal version of split tile log
void bgRmSplitTileLog(const rect_t &r, const cv::Mat &img, CostFunction *cfunc,
                      double expCost, rect_t &newt1, rect_t &newt2,
                      std::list<rect_t> &bgPartitions, float acc, int orient) {

    // Gets upper and lower bounds for the expected cost
    long upperCost = expCost + expCost * acc;
    long lowerCost = expCost - expCost * acc;

    ///////////////////////////////////////////////////////////////////////////
    // Horizontal sweeping
    long              pivotxLen = (r.xo - r.xi) / 2;
    long              pivotx    = r.xi + pivotxLen;
    long              cost1h    = cfunc->cost(img, r.yi, r.yo, r.xi, pivotx);
    long              cost2h = cfunc->cost(img, r.yi, r.yo, pivotx + 1, r.xo);
    std::list<rect_t> bgTilesH;
    std::list<rect_t> bgTilesV;
    rect_t tile1h = {.xi = r.xi, .yi = r.yi, .xo = pivotx, .yo = r.yo};
    rect_t tile2h = {.xi = pivotx + 1, .yi = r.yi, .xo = r.xo, .yo = r.yo};

    long areah;

    // std::cout << "lowerCost " << lowerCost << std::endl;
    // std::cout << "upperCost " << upperCost << std::endl;

    if (orient <= 0) {

        std::cout << "[bgRmSplitTileLog] horz expected cost: " << expCost
                  << "\n";

        // Finds through binary search the best horizontal pivot
        while (!between(cost1h, lowerCost, upperCost) &&
               !between(cost2h, lowerCost, upperCost) && pivotxLen > 0) {

            // std::cout << "[bgRmSplitTileLog] Hcost1: " << cost1h << "\n";
            // std::cout << "[bgRmSplitTileLog] Hcost2: " << cost2h << "\n";
            // std::cout << "cost1h " << cost1h << std::endl;
            // std::cout << "cost2h " << cost2h << std::endl;

            // If the expected cost was not achieved, split the pivotLen
            // and try again, moving the pivot closer to the tile with
            // the greatest cost.
            pivotxLen /= 2;
            if (cost1h > cost2h && expCost > cost2h && expCost < cost1h)
                pivotx -= pivotxLen;
            else
                pivotx += pivotxLen;

            // Avoid zero area regions
            if (pivotx - r.xi < 1 || r.xo - pivotx - 1 < 1)
                break;

            tile1h = {.xi = r.xi, .yi = r.yi, .xo = pivotx, .yo = r.yo};
            tile2h = {.xi = pivotx + 1, .yi = r.yi, .xo = r.xo, .yo = r.yo};
            bgTilesH.clear();
            costWithBgRm(img, cfunc, tile1h, tile2h, bgTilesH, cost1h, cost2h);
            // cost1h = cfunc->cost(img, r.yi, r.yo, r.xi, pivotx);
            // cost2h = cfunc->cost(img, r.yi, r.yo, pivotx + 1, r.xo);
        }

        // Calculate difference between areas of new tiles 1 and 2
        areah = abs((r.yo - r.yi) * (r.xo + r.xi + 2 * pivotx));
    }

    ///////////////////////////////////////////////////////////////////////////
    // Vertical sweeping
    long   pivotyLen = (r.yo - r.yi) / 2;
    long   pivoty    = r.yi + pivotyLen;
    long   cost1v    = cfunc->cost(img, r.yi, pivoty, r.xi, r.xo);
    long   cost2v    = cfunc->cost(img, pivoty + 1, r.yo, r.xi, r.xo);
    long   areav;
    rect_t tile1v = {.xi = r.xi, .yi = r.yi, .xo = r.xo, .yo = pivoty};
    rect_t tile2v = {
        .xi = r.xi + 1, .yi = std::max(0l, pivoty + 1), .xo = r.xo, .yo = r.yo};

    if (orient >= 0) {

        std::cout << "[bgRmSplitTileLog] vert expected cost: " << expCost
                  << "\n";

        // Finds through binary search the best vertical pivot
        while (!between(cost1v, lowerCost, upperCost) &&
               !between(cost2v, lowerCost, upperCost) && pivotyLen > 0) {

            // std::cout << "[bgRmSplitTileLog] Vcost1: " << cost1v << "\n";
            // std::cout << "[bgRmSplitTileLog] Vcost2: " << cost2v << "\n";

            // If the expected cost was not achieved, split the pivotLen
            // and try again, moving the pivot closer to the tile with
            // the greatest cost.
            pivotyLen /= 2;
            if (cost1v > cost2v && expCost > cost2v && expCost < cost1v)
                pivoty -= pivotyLen;
            else
                pivoty += pivotyLen;

            // Avoid zero area regions
            if (pivoty - r.yi < 1 || r.yo - pivoty - 1 < 1)
                break;

            tile1v = {.xi = r.xi, .yi = r.yi, .xo = r.xo, .yo = pivoty};
            tile2v = {.xi = r.xi + 1,
                      .yi = std::max(0l, pivoty + 1),
                      .xo = r.xo,
                      .yo = r.yo};
            bgTilesV.clear();
            costWithBgRm(img, cfunc, tile1v, tile2v, bgTilesV, cost1v, cost2v);
            // cost1v = cfunc->cost(img, r.yi, pivoty, r.xi, r.xo);
            // cost2v =
            //     cfunc->cost(img, std::max(0l, pivoty + 1), r.yo, r.xi, r.xo);
        }

        // Calculate difference between areas of new tiles 1 and 2
        areav = abs((r.xo - r.xi) * (r.yo + r.yi + 2 * pivoty));
    }

    ///////////////////////////////////////////////////////////////////////////
    // Assigns the regions coordinates
    // newt1.xi = r.xi; // newt1 is left and newt2 is right
    // newt1.yi = r.yi;
    // newt2.xo = r.xo; // newt1 is left and newt2 is right
    // newt2.yo = r.yo;

    // Checks whether vertical or horizontal sweep results were better
    if (orient < 0 || (orient == 0 && areah < areav)) { // horizontal
        // newt1.xo = pivotx;
        // newt2.xi = pivotx + 1;
        // newt1.yo = r.yo;
        // newt2.yi = r.yi;
        newt1 = tile1h;
        newt2 = tile2h;
        bgPartitions.insert(bgPartitions.begin(), bgTilesH.begin(),
                            bgTilesH.end());
    } else { // vertical
        // newt1.xo = r.xo;
        // newt2.xi = r.xi;
        // newt1.yo = pivoty;
        // newt2.yi = pivoty + 1;
        newt1 = tile1v;
        newt2 = tile2v;
        bgPartitions.insert(bgPartitions.begin(), bgTilesV.begin(),
                            bgTilesV.end());
    }
}

/*****************************************************************************/
/**                            Other Algorithms                             **/
/*****************************************************************************/

int primes[] = {2,   3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,
                41,  43,  47,  53,  59,  61,  67,  71,  73,  79,  83,  89,
                97,  101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151,
                157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223,
                227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
                283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359,
                367, 373, 379, 383, 389, 397, 401, 409};

// Returns a list of all multiples of a given number
// Does not add 1
std::list<int> getMultiples(int n) {
    std::list<int> multiples;
    // multiples.push_back(1);
    int i = 0;
    while (n != 1 && i < 80) {
        if (n % primes[i] == 0) {
            multiples.push_back(primes[i]);
            n = n / primes[i];
            i = 0;
        } else
            i++;
    }

    return multiples;
}
