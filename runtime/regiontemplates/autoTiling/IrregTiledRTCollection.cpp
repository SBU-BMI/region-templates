#include "IrregTiledRTCollection.h"

/*****************************************************************************/
/***************************** Helper functions ******************************/
/*****************************************************************************/
/*****************************************************************************/

// This representation makes the algorithms easier to implement and understand
typedef struct rect_t {
    rect_t() : isBg(false) {}
    rect_t(int64_t ixi, int64_t iyi, int64_t ixo, int64_t iyo) : 
        xi(ixi), yi(iyi), xo(ixo), yo(iyo), isBg(false) {}
    rect_t(int64_t ixi, int64_t iyi, int64_t ixo, int64_t iyo, bool bg) :
        xi(ixi), yi(iyi), xo(ixo), yo(iyo), isBg(bg) {}
    
    bool operator==(const rect_t r) const{
        if(r.xi == xi && r.xo == xo 
            && r.yi == yi && r.yo == yo 
            && r.isBg == isBg) return true;
        else return false;
    }

    bool operator!=(const rect_t r) const{
        return !(*this == r);
    }

    int64_t xi, yi;
    int64_t xo, yo;
    bool isBg;
} rect_t;

struct rect_tCompX{
    bool operator()(const rect_t& a, const rect_t& b) {
        return a.xi < b.xi || (a.xi == b.xi && 
            (a.xo != b.xo || a.yi != b.yi || a.yo != b.yo));
    }
};

struct rect_tCompY{
    bool operator()(const rect_t& a, const rect_t& b) {
        return a.yo < b.yo || (a.yo == b.yo && 
            (a.xo != b.xo || a.yi != b.yi || a.xi != b.xi));
    }
};

inline bool isInsideNI(int64_t x, int64_t y, rect_t r2) {
    return r2.xi < x && r2.yi < y && r2.xo > x && r2.yo > y;
}

inline bool isInsideNI(rect_t r1, rect_t r2) {
    return isInsideNI(r1.xi, r1.yi, r2) && isInsideNI(r1.xo, r1.yo, r2);
}

inline bool overlaps(rect_t r1, rect_t r2) {
    return isInsideNI(r1.xi, r1.yi, r2) || isInsideNI(r1.xi, r1.yo, r2) ||
        isInsideNI(r1.xo, r1.yi, r2) || isInsideNI(r1.xo, r1.yo, r2) ||
        isInsideNI(r2.xi, r2.yi, r1) || isInsideNI(r2.xi, r2.yo, r1) ||
        isInsideNI(r2.xo, r2.yi, r1) || isInsideNI(r2.xo, r2.yo, r1);
}

void printRect(rect_t r) {
    std::cout << r.xi << ":" << r.xo << "," << r.yi << ":" << r.yo;
}

/*****************************************************************************/
/**                             Inside Overlap                              **/
/*****************************************************************************/

inline bool isInside(int64_t x, int64_t y, rect_t r2) {
    return r2.xi <= x && r2.yi <= y && r2.xo >= x && r2.yo >= y;
}

inline bool isInside(rect_t r1, rect_t r2) {
    return isInside(r1.xi, r1.yi, r2) && isInside(r1.xo, r1.yo, r2);
}

void removeInsideOvlp(std::list<rect_t>& output) {
    // first create an array for parallelization
    int outS = output.size();
    rect_t outArray[outS];
    std::copy(output.begin(), output.end(), outArray);

    // create an array of tags for non repeated regions
    bool outArrayNR[outS];
    for (int i=0; i<outS; i++) {outArrayNR[i] = false;}

    // compare each element with each other, checking if it's inside any
    #pragma omp parallel for
    for (int i=0; i<outS; i++) {
        int j;
        for (j=0; j<outS; j++) {
            if (isInside(outArray[i], outArray[j]) && i!=j)
                break;
        }
        // check if no other bigger region was found
        if (j == outS) {
            outArrayNR[i] = true;
        }
    }

    // clear the old elements and add only the unique regions
    output.clear();
    for (int i=0; i<outS; i++) {
        if (outArrayNR[i])
            output.push_back(outArray[i]);
    }
}

/*****************************************************************************/
/**                              Side Overlap                               **/
/*****************************************************************************/

inline bool hOvlp(rect_t big, rect_t small) {
    return big.yi <= small.yi && big.yo >= small.yo 
        && ((big.xi < small.xi && big.xo > small.xi)
        || (big.xi < small.xo && big.xo > small.xo));
}

inline bool vOvlp(rect_t big, rect_t small) {
    return big.xi <= small.xi && big.xo >= small.xo 
        && ((big.yi < small.yi && big.yo > small.yi)
        || (big.yi < small.yo && big.yo > small.yo));
}

void removeSideOvlp(std::list<rect_t>& output) {
    // first create an array for parallelization
    int outS = output.size();
    rect_t outArray[outS];
    std::copy(output.begin(), output.end(), outArray);

    // compare each element with each other, checking if it's inside any
    #pragma omp parallel for
    for (int i=0; i<outS; i++) {
        int big=-1, small=-1;
        for (int j=0; j<outS; j++) {

            // Checks if there is a horizontal overlapping
            if (hOvlp(outArray[i], outArray[j]) 
                || hOvlp(outArray[i], outArray[j])) {

                // find which is the big one
                big = i; small = j;
                if (outArray[i].yi > outArray[j].yi) {
                    big = j; small = i;
                }

                // remove the overlapping of small with big
                if (outArray[small].xi > outArray[big].xi) // big left
                    outArray[small].xi = outArray[big].xo;
                else // big on the right
                    outArray[small].xo = outArray[big].xi;
            }
            // Checks if there is a vertical overlapping
            if (vOvlp(outArray[i], outArray[j]) 
                || vOvlp(outArray[i], outArray[j])) {

                // find which is the big one
                big = i; small = j;
                if (outArray[i].xi > outArray[j].xi) {
                    big = j; small = i;
                }

                // remove the overlapping of small with big
                if (outArray[small].yi > outArray[big].yi) // big up
                    outArray[small].yi = outArray[big].yo;
                else // big is the down region
                    outArray[small].yo = outArray[big].yi;
            }
        }
    }

    // clear the old elements and add only the unique regions
    output.clear();
    for (int i=0; i<outS; i++) {
        // Only adds elements which have an actual area > 0
        if (outArray[i].xi != outArray[i].xo && outArray[i].yi != outArray[i].yo)
            output.push_back(outArray[i]);
    }
}

/*****************************************************************************/
/**                            Diagonal Overlap                             **/
/*****************************************************************************/

inline int64_t area (rect_t r) {
    return (r.xo-r.xi) * (r.yo-r.yi);
}

void removeDiagOvlp(std::list<rect_t>& ovlpCand, std::list<rect_t>& nonMod) {
    // first create an array for parallelization
    int outS = ovlpCand.size();
    rect_t outArray[outS];
    std::copy(ovlpCand.begin(), ovlpCand.end(), outArray);

    // array for the replaced 3 new blocks for every initial block
    rect_t outRepArray[outS][3];

    // marker array for showing which blocks were to be replaced
    bool outRepArrayR[outS];
    bool outRepArrayS[outS];
    for (int i=0; i<outS; i++) {
        outRepArrayR[i] = false;
        outRepArrayS[i] = false;
    }

    // compare each element with each other, checking if it's inside any
    #pragma omp parallel for
    for (int i=0; i<outS; i++) {
        for (int j=0; j<outS; j++) {
            rect_t b = outArray[i];
            rect_t s = outArray[j];

            // break the big block if there is a diagonal overlap
            if (b.xi > s.xi && b.xi < s.xo && b.yo > s.yi && b.yo < s.yo) {
                // big on upper right diagonal
                // only break down if <i> is the big block of a diagonal overlap
                if (area(outArray[i]) < area(outArray[j])) {
                    outRepArrayS[i] = true;
                    continue;
                }
                outRepArrayR[i] = true;
                outRepArray[i][0] = {b.xi, b.yi, s.xo, s.yi};
                outRepArray[i][1] = {s.xo, b.yi, b.xo, s.yi};
                outRepArray[i][2] = {s.xo, s.yi, b.xo, b.yo};
            } else if (b.xo > s.xi && b.xo < s.xo && b.yo > s.yi && b.yo < s.yo) {
                // big on upper left
                // only break down if <i> is the big block of a diagonal overlap
                if (area(outArray[i]) < area(outArray[j])) {
                    outRepArrayS[i] = true;
                    continue;
                }
                outRepArrayR[i] = true;
                outRepArray[i][0] = {b.xi, b.yi, s.xi, s.yi};
                outRepArray[i][1] = {s.xi, b.yi, b.xo, s.yi};
                outRepArray[i][2] = {b.xi, s.yi, s.xi, b.yo};
            } else if (b.xi > s.xi && b.xi < s.xo && b.yi > s.yi && b.yi < s.yo) {
                // big on bottom right
                // only break down if <i> is the big block of a diagonal overlap
                if (area(outArray[i]) < area(outArray[j])) {
                    outRepArrayS[i] = true;
                    continue;
                }
                outRepArrayR[i] = true;
                outRepArray[i][0] = {s.xo, b.yi, b.xo, s.yo};
                outRepArray[i][1] = {b.xi, s.yo, s.xo, b.yo};
                outRepArray[i][2] = {s.xo, s.yo, b.xo, b.yo};
            } else if (b.xo > s.xi && b.xo < s.xo && b.yi > s.yi && b.yi < s.yo) {
                // big on bottom left
                // only break down if <i> is the big block of a diagonal overlap
                if (area(outArray[i]) < area(outArray[j])) {
                    outRepArrayS[i] = true;
                    continue;
                }
                outRepArrayR[i] = true;
                outRepArray[i][0] = {b.xi, b.yi, s.xi, s.yo};
                outRepArray[i][1] = {s.xi, s.yo, b.xo, b.yo};
                outRepArray[i][2] = {b.xi, s.yo, s.xi, b.yo};
            }
        }
    }

    // clear the old elements and add only the unique regions
    ovlpCand.clear();
    for (int i=0; i<outS; i++) {
        if (outRepArrayR[i]) {
            ovlpCand.push_back(outRepArray[i][0]);
            ovlpCand.push_back(outRepArray[i][1]);
            ovlpCand.push_back(outRepArray[i][2]);
        } else if (outRepArrayS[i])
            ovlpCand.push_back(outArray[i]);
        else
            nonMod.push_back(outArray[i]);

    }
}

/*****************************************************************************/
/**                          Background Generator                           **/
/*****************************************************************************/

void printRegions(cv::Mat final, std::list<rect_t> output, int i) {
    
    for (std::list<rect_t>::iterator r=output.begin(); r!=output.end(); r++) {
        // draw areas for verification
        if (r->isBg) {
            // cv::rectangle(final, cv::Point(r->xi,r->yi), 
            //     cv::Point(r->xo,r->yo),(0,0,0),3);
            cv::rectangle(final, cv::Point(r->xi,r->yi), 
                cv::Point(r->xo,r->yo),(0,0,0),1);
            printRect(*r);
            std::cout << std::endl;
        }
    }

    cv::imwrite("./maskf" + std::to_string(i) + ".png", final);
}

// yi = oldY
// yo = min(r,cur)
// xo = width
void newBlocks(std::list<rect_t>& out, std::multiset<rect_t,rect_tCompX> cur, 
    int64_t yi, int64_t yo, int64_t xo) {

    if (yo-yi == 1)
        return;

    int64_t xi = 0;
    for (rect_t r : cur) {
        rect_t newR = {xi, yi, r.xi, yo, true};
        // don't add lines i.e. zero area rect
        if (newR.xi != newR.xo && newR.yi != newR.yo)
            out.push_back(newR);
        xi = r.xo;
    }

    // add last
    rect_t newR = {xi, yi, xo, yo, true};
    if (newR.xi != newR.xo && newR.yi != newR.yo)
        out.push_back(newR);
}

void generateBackground(std::list<rect_t>& dense, 
    std::list<rect_t>& output, int64_t maxCols, int64_t maxRows) {

    int64_t oldY = 0;
    std::multiset<rect_t,rect_tCompX> curX;
    std::multiset<rect_t,rect_tCompY> curY;

    // sort dense by the top of each area
    dense.sort([](const rect_t& a, const rect_t& b) {return a.yi < b.yi;});

    while (!curY.empty() || !dense.empty()) {
        // get the heads of dense, if there is one
        rect_t r;
        if (!dense.empty())
            r = *(dense.begin());

        // check if the current y is from the beginning of the end of a rect
        // two comparisons are necessary since there may be a beginning with
        // an end on the same coordinate
        if (curY.empty() || (!dense.empty() && r.yi <= curY.begin()->yo)) {
            // make the new rect blocks
            newBlocks(output, curX, oldY, r.yi, maxCols);

            // update cur structure
            oldY = r.yi;
            curX.emplace(r);
            curY.emplace(r);

            // remove the new node from dense
            dense.erase(dense.begin());
        } else if (dense.empty() || (!dense.empty() 
            && r.yi > curY.begin()->yo)) {

            // make the new rect blocks
            newBlocks(output, curX, oldY, curY.begin()->yo, maxCols);

            // update cur structure and remove top cur
            oldY = curY.begin()->yo;
            curX.erase(*curY.begin());
            curY.erase(curY.begin());
        }
    }

    // Creates the last tile
    output.push_back({0, oldY, maxCols, maxRows, true});
}

void bgMerging(std::list<rect_t>& output) {
    bool merged = true;

    while (merged) {
        merged = false;
        for (std::list<rect_t>::iterator i = output.begin(); 
            i != output.end(); i++) {
            for (std::list<rect_t>::iterator j = output.begin(); 
                j != output.end(); j++) {        

                // We cannot merge the same regions or non-bg regions
                if (*i != *j && i->isBg && j->isBg) {
                    // Check for vertical bordering (i on top of j)
                    if (i->xi == j->xi && (i->yo == j->yi || i->yo == j->yi-1) 
                            && i->xo == j->xo) {
                        i->yo = j->yo;
                        j = output.erase(j);
                        merged = true;
                    }

                    // Check for horizontal bordering (i left to j)
                    if ((i->xo == j->xi || i->xo == j->xi-1) && i->yi == j->yi 
                            && i->yo == j->yo) {
                        i->xo = j->xo;
                        j = output.erase(j);
                        merged = true;
                    }
                }
            }
        }
    }
}

/*****************************************************************************/
/**                          Dense Regions Tiling                           **/
/*****************************************************************************/

inline int64_t cost(const cv::Mat& img) {
    return cv::sum(img)[0];
}

inline int64_t cost(const cv::Mat& img, const rect_t& r) {
    return cv::sum(img(cv::Range(r.yi, r.yo), cv::Range(r.xi, r.xo)))[0];
}

inline int64_t cost(const cv::Mat& img, int64_t yi, int64_t yo, 
        int64_t xi, int64_t xo) {
    return cv::sum(img(cv::Range(yi, yo), cv::Range(xi, xo)))[0];
}

inline bool between(int64_t x, int64_t init, int64_t end) {
    return x >= init && x <= end;
}

// Functor for sorting a container of rect_t using the added cost of said
// region on the img parameter.
// NOTE: when instantiating container, use double parenthesis to avoid 
// compiler confusion of the declared object with a function:
// Cont<t1, rect_tCostFunct> obj((rect_tCostFunct(img)))
struct rect_tCostFunct{
    const cv::Mat& img;
    rect_tCostFunct(const cv::Mat& img) : img(img) {}
    bool operator()(const rect_t& a, const rect_t& b) {
        return cost(img, a) > cost(img, b);
    }
};

// Performs a binary sweep search across the input tile image, searching for 
// a split on which the goal cost is found within an acc margin of error.
// At least one of the new tiles must have the goal cost. Both a vertical
// and a horizontal sweep are performed, being returned the pair of regions
// with the smallest difference between areas.
// Orient: 0 = both, -1 = horizontal only, +1 = vetical only
void splitTileLog(const rect_t& r, const cv::Mat& img, int expCost, 
    rect_t& newt1, rect_t& newt2, float acc=0.2, int orient = 0) {

    // Gets upper and lower bounds for the expected cost
    int upperCost = expCost + expCost*acc;
    int lowerCost = expCost - expCost*acc;

    ///////////////////////////////////////////////////////////////////////////
    // Horizontal sweeping
    int pivotxLen = (r.xo - r.xi)/2;
    int pivotx = r.xi + pivotxLen;
    int cost1h = cost(img, r.yi, r.yo, r.xi, pivotx);
    int cost2h = cost(img, r.yi, r.yo, pivotx+1, r.xo);
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

            cost1h = cost(img, r.yi, r.yo, r.xi, pivotx);
            cost2h = cost(img, r.yi, r.yo, pivotx+1, r.xo);
        }

        // Calculate difference between areas of new tiles 1 and 2
        areah = abs((r.yo - r.yi)*(r.xo + r.xi + 2*pivotx));
    }

    ///////////////////////////////////////////////////////////////////////////
    // Vertical sweeping
    int pivotyLen = (r.yo - r.yi)/2;
    int pivoty = r.yi + pivotyLen;
    int cost1v = cost(img, r.yi, pivoty, r.xi, r.xo);
    int cost2v = cost(img, pivoty+1, r.yo, r.xi, r.xo);
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

            cost1v = cost(img, r.yi, pivoty, r.xi, r.xo);
            cost2v = cost(img, pivoty+1, r.yo, r.xi, r.xo);
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

void splitTileCost(const cv::Mat& img, rect_t initial, rect_t& newL, 
    rect_t& newR, bool orient) {

    int cost = cv::sum(img)[0]/2;
    splitTileLog(initial, img, cost, newL, newR, 0.2, orient?1:-1);
}

/*****************************************************************************/
/**                         Autotiling Algorithms                           **/
/*****************************************************************************/

void tileDenseFromBG(cv::Mat& mask, std::list<rect_t>& dense, 
    std::list<rect_t>& output, const cv::Mat* input = NULL) {

    // merge regions of interest together and mark them separately
    cv::Mat stats, centroids;
    cv::connectedComponentsWithStats(mask, mask, stats, centroids);

    // Get number of initial dense regions
    double maxLabel;
    cv::minMaxLoc(mask, NULL, &maxLabel);

    // generate the list of dense areas
    std::list<rect_t> ovlpCand;
    int minArea = 500;
    // int minArea = 0;
    for (int i=1; i<=maxLabel; i++) { // i=1 ignores background
        // std::cout << "area: " 
        //    << stats.at<int>(i, cv::CC_STAT_AREA) << std::endl;
        // Ignores small areas
        if (stats.at<int>(i, cv::CC_STAT_AREA) > minArea) {
            int xi = stats.at<int>(i, cv::CC_STAT_LEFT);
            int yi = stats.at<int>(i, cv::CC_STAT_TOP);
            int xw = stats.at<int>(i, cv::CC_STAT_WIDTH);
            int yh = stats.at<int>(i, cv::CC_STAT_HEIGHT);
            rect_t rr = {.xi=xi, .yi=yi, .xo=xi+xw, .yo=yi+yh};
            ovlpCand.push_back(rr);
        }
    }

// #ifdef DEBUG
    std::cout << "[tileDenseFromBG] Tiling image size: " 
        << mask.cols << "x" << mask.rows << std::endl;
    std::cout << "[tileDenseFromBG] Initial dense regions: " 
        << maxLabel << std::endl;
// #endif
    
    // keep trying to remove overlapping regions until there is none
    while (!ovlpCand.empty()) {
        // remove regions that are overlapping within another bigger region
        removeInsideOvlp(ovlpCand);

        // remove the overlap of two regions, side by side (vert and horz)
        removeSideOvlp(ovlpCand);

        // remove the diagonal overlaps
        removeDiagOvlp(ovlpCand, dense);
    }

    // Creates a copy of the dense regions which can be consumed
    std::list<rect_t> denseTmp(dense);
    
    // generate the background regions
    generateBackground(denseTmp, output, mask.cols, mask.rows);

    // Perform merging of background areas
    bgMerging(output);

// #ifdef DEBUG
    std::cout << "[tileDenseFromBG] Total regions to process: " 
        << output.size() << std::endl;
    cv::Mat final;
    if (input != NULL)
        final = input->clone();

    std::list<cv::Rect_<int64_t> > cvOutput;
    for (std::list<rect_t>::iterator r=output.begin(); r!=output.end(); r++) {
        // draw areas for verification
        if (input != NULL) {
            cv::rectangle(final, cv::Point(r->xi,r->yi), 
                cv::Point(r->xo,r->yo),(0,0,0),3);
        }
    }

    if (input != NULL) {
        cv::imwrite("./maskNoBg.png", final);
    }
// #endif
}

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
            std::cout << "[IrregTiledRTCollection] Tile too small to split."
                << std::endl;
            exit(-1);
        }
        if (type == LIST_ALG_HALF)
            splitTileLog(*dIt, img, cost(img, *dIt)/2, newt1, newt2);
        else if (type == LIST_ALG_EXPECT)
            splitTileLog(*dIt, img, avgCost, newt1, newt2);
        else {
            std::cout << "[IrregTiledRTCollection] Bad listCutting alg type: "
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

void kdTreeCutting(const cv::Mat& img, std::list<rect_t>& dense, 
    int nTiles, TilerAlg_t type) {

    // Calculates the number of cuts tree levels required
    int levels = ceil(log(nTiles/dense.size())/log(2));
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
                    std::cout << "[IrregTiledRTCollection] Bad kdTreeCutting"
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

void stddev(std::list<rect_t> rs, const cv::Mat& img, std::string name) {
    float mean = 0;
    for (rect_t r : rs)
        mean += cost(img, r);
    mean /= rs.size();

    float var = 0;
    for (rect_t r : rs)
        var += pow(cost(img, r)-mean, 2);
    
    std::cout << "[PROFILING][" << name << "]" 
        << (sqrt(var/(rs.size()-1))) << std::endl;
}

/*****************************************************************************/
/****************************** Class methods ********************************/
/*****************************************************************************/
/*****************************************************************************/

IrregTiledRTCollection::IrregTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int border, BGMasker* bgm, 
    PreTilerAlg_t preTier, TilerAlg_t tilingAlg, int nTiles) 
        : TiledRTCollection(name, refDDRName, tilesPath) {

    this->border = border;
    this->bgm = bgm;
    this->nTiles = nTiles;
    this->preTier = preTier;
    this->tilingAlg = tilingAlg;
}

void IrregTiledRTCollection::customTiling() {
    std::string drName;
    // Go through all images
    for (int i=0; i<initialPaths.size(); i++) {
        // Create the list of tiles for the current image
        std::list<cv::Rect_<int64_t>> rois;

        // Check whether the input file is a svs file
        bool isSvs = isSVS(initialPaths[i]);

        // Open image for tiling
        int64_t w = -1;
        int64_t h = -1;
        openslide_t* osr;
        int32_t osrMinLevel = -1;
        int32_t osrMaxLevel = 0; // svs standard: max level = 0
        float ratiow;
        float ratioh; 
        cv::Mat maskMat;

        // Opens svs input file
        osr = openslide_open(initialPaths[i].c_str());

        // Gets info of largest image
        openslide_get_level0_dimensions(osr, &w, &h);
        ratiow = w;
        ratioh = h;

        // Opens smallest image as a cv mat
        osrMinLevel = openslide_get_level_count(osr) - 1; // last level
        openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
        cv::Rect_<int64_t> roi(0, 0, w, h);
        osrRegionToCVMat(osr, roi, osrMinLevel, maskMat);

        // Calculates the ratio between largest and smallest 
        // images' dimensions for later conversion
        ratiow /= w;
        ratioh /= h;

        // Perfeorms the preTiling if required
        std::list<rect_t> finalTiles;
        std::list<rect_t> preTiledAreas;
        cv::Mat thMask = bgm->bgMask(maskMat);
        switch (this->preTier) {
            case DENSE_BG_SEPARATOR: {
                tileDenseFromBG(thMask, preTiledAreas, finalTiles, &maskMat);
                break;
            }
            case NO_PRE_TILER: { // just return the full image as a single tile
                preTiledAreas.push_back({0, 0, maskMat.cols, maskMat.rows});
                break;
            }
        }
        
        // Ensure that thMask is a binary mat
        thMask.convertTo(thMask, CV_8U);
        cv::threshold(thMask, thMask, 0, 255, cv::THRESH_BINARY);

        if (preTiledAreas.size() >= this->nTiles) {
            std::cout << "[IrregTiledRTCollection] No dense tiling"
                << " performed, wanted " << this->nTiles 
                << " but already have " << preTiledAreas.size()
                << " tiles." << std::endl;
        } else {
            switch (this->tilingAlg) {
                case LIST_ALG_EXPECT: {
                    listCutting(thMask, preTiledAreas, 
                        this->nTiles, this->tilingAlg);
                    break;
                }
                case LIST_ALG_HALF: {
                    listCutting(thMask, preTiledAreas, 
                        this->nTiles, this->tilingAlg);
                    break;
                }
                case KD_TREE_ALG_AREA: {
                    kdTreeCutting(thMask, preTiledAreas,
                        this->nTiles, this->tilingAlg);
                    break;
                }
                case KD_TREE_ALG_COST: {
                    kdTreeCutting(thMask, preTiledAreas,
                        this->nTiles, this->tilingAlg);
                    break;
                }
            }

            // Gets std-dev of dense tiles' sizes
            stddev(preTiledAreas, thMask, "DENSESTDDEV");

        }
        // Send resulting tiles to the final output
        finalTiles.insert(finalTiles.end(), 
            preTiledAreas.begin(), preTiledAreas.end());

        // Gets std-dev of all tiles' sizes
        stddev(finalTiles, thMask, "ALLSTDDEV");

        // Convert rect_t to cv::Rect_ and add borders
        std::list<cv::Rect_<int64_t> > tiles;
        for (std::list<rect_t>::iterator r=finalTiles.begin(); 
                r!=finalTiles.end(); r++) {

            r->xi = std::max(r->xi-this->border, (int64_t)0);
            r->xo = std::min(r->xo+this->border, (int64_t)thMask.cols);
            r->yi = std::max(r->yi-this->border, (int64_t)0);
            r->yo = std::min(r->yo+this->border, (int64_t)thMask.rows);

            tiles.push_back(cv::Rect_<int64_t>(
                r->xi, r->yi, r->xo-r->xi, r->yo-r->yi));

// #ifdef DEBUG
            cv::rectangle(maskMat, cv::Point(r->xi,r->yi), 
                cv::Point(r->xo,r->yo),(0,0,0),3);
// #endif
        }

// #ifdef DEBUG
        cv::imwrite("./maskf.png", maskMat);
// #endif

        // Actually tile the image given the list of ROIs
        int drId=0;
        for (cv::Rect_<int64_t> tile : tiles) {
            // Creates the tile name for the original svs file, if lazy
            std::string path = this->tilesPath;

            // Converts the tile roi for the bigger image
            DataRegion *dr = new DenseDataRegion2D();
            tile.x *= ratiow;
            tile.width *= ratiow;
            tile.y *= ratioh;
            tile.height *= ratioh;

            // Creates the dr as a svs data region for
            // lazy read/write of input file
            dr->setRoi(tile);
            dr->setSvs();
            
            // Create new RT tile from roi
            std::string drName = "t" + to_string(drId);
            dr->setName(drName);
            dr->setId(refDDRName);
            dr->setInputType(DataSourceType::FILE_SYSTEM);
            dr->setIsAppInput(true);
            dr->setOutputType(DataSourceType::FILE_SYSTEM);
            dr->setInputFileName(path);
            RegionTemplate* newRT = new RegionTemplate();
            newRT->insertDataRegion(dr);
            newRT->setName(name);

            // Add the tile and the RT to the internal containers
            rois.push_back(tile);
            this->rts.push_back(
                std::pair<std::string, RegionTemplate*>(drName, newRT));
            drId++;
        }

        // Close .svs file
        if (isSvs) {
            openslide_close(osr);
        }

        // Add the current image tiles to the tiles vector
        this->tiles.push_back(rois);
    }
}
