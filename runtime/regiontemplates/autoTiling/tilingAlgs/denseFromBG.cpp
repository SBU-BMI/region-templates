#include "denseFromBG.h"

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
/**                               Main Tiler                                **/
/*****************************************************************************/

void tileDenseFromBG(cv::Mat& mask, std::list<rect_t>& dense, 
    std::list<rect_t>& output, const cv::Mat* input) {

    // merge regions of interest together and mark them separately
    cv::Mat stats, centroids;
    cv::connectedComponentsWithStats(mask, mask, stats, centroids);

    // Get number of initial dense regions
    double maxLabel;
    cv::minMaxLoc(mask, NULL, &maxLabel);

    // generate the list of dense areas
    std::list<rect_t> ovlpCand;
    float f = 0.01;
    int minArea = mask.cols*mask.rows*f;
    for (int i=1; i<=maxLabel; i++) { // i=1 ignores background
        // std::cout << "area: " 
        //    << stats.at<int>(i, cv::CC_STAT_AREA) << std::endl;
        // Ignores small areas
        if (stats.at<int>(i, cv::CC_STAT_AREA) > 500) {
            int xi = stats.at<int>(i, cv::CC_STAT_LEFT);
            int yi = stats.at<int>(i, cv::CC_STAT_TOP);
            int xw = stats.at<int>(i, cv::CC_STAT_WIDTH);
            int yh = stats.at<int>(i, cv::CC_STAT_HEIGHT);
            if (xw > 1 && yh > 1 && yh*xw > minArea) {
                rect_t rr = {.xi=xi, .yi=yi, .xo=xi+xw, .yo=yi+yh};
                ovlpCand.push_back(rr);
            }
        }
    }

    // Checks if at least one dense region is existent
    if (ovlpCand.size() == 0) {
        std::cout << "[tileDenseFromBG] Bad background threshold parameters, "
            << "no dense regions found" << std::endl;
        exit(0);
    }

#ifdef DEBUG
    std::cout << "[tileDenseFromBG] Tiling image size: " 
        << mask.cols << "x" << mask.rows << std::endl;
    std::cout << "[tileDenseFromBG] Initial dense regions: " 
        << maxLabel << std::endl;
#endif
    
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

#ifdef DEBUG
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
#endif
}
