#include "tilingUtil.h"

std::list<rect_t> toMyRectT(std::list<cv::Rect_<uint64_t>> from) {
    std::list<rect_t> to;
    for (auto r : from) {
        to.emplace_back(r.x, r.y, r.x + r.width - 1, r.y + r.height - 1);
    }
    return to;
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
