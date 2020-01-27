#include "tilingUtil.h"

/*****************************************************************************/
/**                            I/O and Profiling                            **/
/*****************************************************************************/

// prints the std dev of a tile inside an image. Used for profiling
void stddev(std::list<rect_t> rs, const cv::Mat& img, std::string name, CostFunction* cfunc) {
    float mean = 0;
    for (rect_t r : rs)
        mean += cfunc->cost(img, r.yi, r.yo, r.xi, r.xo);
    mean /= rs.size();

    float var = 0;
    for (rect_t r : rs)
        var += pow(cfunc->cost(img, r.yi, r.yo, r.xi, r.xo)-mean, 2);
    
    std::cout << "[PROFILING][STDDEV][" << name << "]" 
        << (sqrt(var/(rs.size()-1))) << std::endl;
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
void splitTileLog(const rect_t& r, const cv::Mat& img, CostFunction* cfunc,
    int expCost, rect_t& newt1, rect_t& newt2, float acc, int orient) {

    // Gets upper and lower bounds for the expected cost
    int upperCost = expCost + expCost*acc;
    int lowerCost = expCost - expCost*acc;

    ///////////////////////////////////////////////////////////////////////////
    // Horizontal sweeping
    int pivotxLen = (r.xo - r.xi)/2;
    int pivotx = r.xi + pivotxLen;
    int cost1h = cfunc->cost(img, r.yi, r.yo, r.xi, pivotx);
    int cost2h = cfunc->cost(img, r.yi, r.yo, pivotx+1, r.xo);
    int areah;
    if (orient <= 0) {

        // Finds through binary search the best horizontal pivot
        while (!between(cost1h, lowerCost, upperCost) 
            && !between(cost2h, lowerCost, upperCost)
            && pivotxLen > 0) {

            // If the expected cost was not achieved, split the pivotLen
            // and try again, moving the pivot closer to the tile with
            // the greatest cost.
            pivotxLen /= 2;
            if (cost1h > cost2h)
                pivotx -= pivotxLen;
            else
                pivotx += pivotxLen;

            cost1h = cfunc->cost(img, r.yi, r.yo, r.xi, pivotx);
            cost2h = cfunc->cost(img, r.yi, r.yo, pivotx+1, r.xo);
        }

        // Calculate difference between areas of new tiles 1 and 2
        areah = abs((r.yo - r.yi)*(r.xo + r.xi + 2*pivotx));
    }

    ///////////////////////////////////////////////////////////////////////////
    // Vertical sweeping
    int pivotyLen = (r.yo - r.yi)/2;
    int pivoty = r.yi + pivotyLen;
    int cost1v = cfunc->cost(img, r.yi, pivoty, r.xi, r.xo);
    int cost2v = cfunc->cost(img, pivoty+1, r.yo, r.xi, r.xo);
    int areav;
    if (orient >= 0) {

        // Finds through binary search the best vertical pivot
        while (!between(cost1v, lowerCost, upperCost) 
            && !between(cost2v, lowerCost, upperCost)
            && pivotyLen > 0) {

            // If the expected cost was not achieved, split the pivotLen
            // and try again, moving the pivot closer to the tile with
            // the greatest cost.
            pivotyLen /= 2;
            if (cost1v > cost2v)
                pivoty -= pivotyLen;
            else
                pivoty += pivotyLen;

            cost1v = cfunc->cost(img, r.yi, pivoty, r.xi, r.xo);
            cost2v = cfunc->cost(img, pivoty+1, r.yo, r.xi, r.xo);
        }

        // Calculate difference between areas of new tiles 1 and 2
        areav = abs((r.xo - r.xi)*(r.yo + r.yi + 2*pivoty));
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
        newt2.xi = pivotx+1;
        newt1.yo = r.yo;
        newt2.yi = r.yi;
    } else { // vertical
        newt1.xo = r.xo;
        newt2.xi = r.xi;
        newt1.yo = pivoty;
        newt2.yi = pivoty+1;
    }
}
