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
        int big, small;
        for (int j=0; j<outS; j++) {
            // check if there is a horizontal overlapping
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
            // check if there is a vertical overlapping
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

// yi = oldY
// yo = min(r,cur)
// xo = width
void newBlocks(std::list<rect_t>& out, std::multiset<rect_t,rect_tCompX> cur, 
    int64_t yi, int64_t yo, int64_t xo) {

    int64_t xi = 0;
    for (rect_t r : cur) {
        rect_t newR = {xi, yi, r.xi, yo, true};
        // don't add lines i.e. zero area rect
        if (newR.xi != newR.xo && newR.yi != newR.yo)
            out.push_back(newR);
        xi = r.xo;
    }

    // add last
    rect_t newR = {xi, yi, xo, yo};
    if (newR.xi != newR.xo && newR.yi != newR.yo)
        out.push_back(newR);
}

void generateBackground(std::list<rect_t>& dense, 
    std::list<rect_t>& output, int64_t maxCols) {

    int64_t oldY = 0;
    std::multiset<rect_t,rect_tCompX> curX;
    std::multiset<rect_t,rect_tCompY> curY;

    // sort dense by the top of each area
    dense.sort([](const rect_t& a, const rect_t& b) { return a.yi < b.yi;});

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
            // dense.sort([](const rect_t& a, const rect_t& b) 
            //     {return a.yi < b.yi;});
        } else if (dense.empty() || (!dense.empty() 
            && r.yi >= curY.begin()->yo)) {

            // make the new rect blocks
            newBlocks(output, curX, oldY, curY.begin()->yo, maxCols);

            // update cur structure and remove top cur
            oldY = curY.begin()->yo;
            curX.erase(*curY.begin());
            curY.erase(curY.begin());
        }
    }
}

void bgMerging(std::list<rect_t>& output) {
    // // sort the ROIs list by the top of each area
    // output.sort([](const rect_t& a, const rect_t& b) { return a.yi < b.yi;});

    for (std::list<rect_t>::iterator i = output.begin(); 
        i != output.end(); i++) {

        for (std::list<rect_t>::iterator j = output.begin(); 
            j != output.end(); j++) {        

            // We cannot merge the same regions or non-bg regions
            if (*i != *j && i->isBg && j->isBg) {
                // Check for vertical bordering (i on top of j)
                if (i->xi == j->xi && i->yo == j->yi && i->xo == j->xo) {
                    i->yo = j->yo;
                    j = output.erase(j);
                }

                // Check for horizontal bordering (i left to j)
                if (i->xo == j->xi && i->yi == j->yi && i->yo == j->yo) {
                    i->xo = j->xo;
                    j = output.erase(j);
                }
            }
        }
        
    }
    
}

/*****************************************************************************/
/**                             Full AutoTiler                              **/
/*****************************************************************************/

std::list<cv::Rect_<int64_t> > autoTiler(cv::Mat& mask, 
    int border, const cv::Mat* input = NULL) {

    std::list<rect_t> output;

    // merge regions of interest together and mark them separately
    cv::Mat stats, centroids;
    cv::connectedComponentsWithStats(mask, mask, stats, centroids);

    double maxLabel;
    cv::minMaxLoc(mask, NULL, &maxLabel);
// #ifdef DEBUG
    std::cout << "[autoTiler] Image size: " 
        << mask.cols << "x" << mask.rows << std::endl;
    std::cout << "[autoTiler] Initial dense regions: " << maxLabel << std::endl;
// #endif
    
    // generate the list of dense areas
    std::list<rect_t> ovlpCand;
    for (int i=1; i<=maxLabel; i++) { // i=1 ignores background
        int xi = stats.at<int>(i, cv::CC_STAT_LEFT);
        int yi = stats.at<int>(i, cv::CC_STAT_TOP);
        int xw = stats.at<int>(i, cv::CC_STAT_WIDTH);
        int yh = stats.at<int>(i, cv::CC_STAT_HEIGHT);
        rect_t rr = {.xi=xi, .yi=yi, .xo=xi+xw, .yo=yi+yh};
        ovlpCand.push_back(rr);
    }

    // keep trying to remove overlapping regions until there is none
    while (!ovlpCand.empty()) {
        // remove regions that are overlapping within another bigger region
        removeInsideOvlp(ovlpCand);

        // remove the overlap of two regions, side by side (vert and horz)
        removeSideOvlp(ovlpCand);

        // remove the diagonal overlaps
        removeDiagOvlp(ovlpCand, output);
    }
    
    // sort the list of dense regions by its y coordinate (using lambda)
    std::list<rect_t> dense(output);
    
    // generate the background regions
    generateBackground(dense, output, mask.cols);

    // Perform merging of background areas
    bgMerging(output);

// #ifdef DEBUG
    std::cout << "[autoTiler] Total regions to process: " 
        << output.size() << std::endl;
    cv::Mat final;
    if (input != NULL)
        final = input->clone();
// #endif

#ifdef DEBUG
    // Verification if there are any overlapping ROIs remaining
    for (std::list<rect_t>::iterator 
        r1=output.begin(); r1!=output.end(); r1++) {

        for (std::list<rect_t>::iterator 
            r2=output.begin(); r2!=output.end(); r2++) {
            
            // Overlap is only checked if the ROIs are different
            if (r1 != r2) {
                if (overlaps(*r1, *r2)) {
                    std::cout << "=========OVERLAP:" << std::endl;
                    std::cout << "r1: (" << r1->xi << "," << r1->yi << "), (" 
                        << r1->xo << "," << r1->yo << ")" << std::endl;
                    std::cout << "r2: (" << r2->xi << "," << r2->yi << "), (" 
                        << r2->xo << "," << r2->yo << ")" << std::endl;
                    // exit(-10);
                }
            }
        }

        // draw areas for verification
        if (input != NULL)
            cv::rectangle(final, cv::Point(r1->xi,r1->yi), 
                cv::Point(r1->xo,r1->yo),(0,0,0),3);
    }
#endif

#ifdef DEBUG
    if (input != NULL)
        cv::imwrite("./maskNoBorder.png", final);
#endif
     
    // add a border to all rect regions and create cv list formated output
    std::list<cv::Rect_<int64_t> > cvOutput;
    for (std::list<rect_t>::iterator r=output.begin(); r!=output.end(); r++) {
        r->xi = std::max(r->xi-border, (int64_t)0);
        r->xo = std::min(r->xo+border, (int64_t)mask.cols);
        r->yi = std::max(r->yi-border, (int64_t)0);
        r->yo = std::min(r->yo+border, (int64_t)mask.rows);

        cvOutput.push_back(cv::Rect_<int64_t>(
            r->xi, r->yi, r->xo-r->xi, r->yo-r->yi));

// #ifdef DEBUG
        // draw areas for verification
        if (input != NULL)
            cv::rectangle(final, cv::Point(r->xi,r->yi), 
                cv::Point(r->xo,r->yo),(0,0,0),3);
// #endif
    }

// #ifdef DEBUG
    if (input != NULL) {
        cv::imwrite("./maskf.png", final);
    }
// #endif

    return cvOutput;
}

/*****************************************************************************/
/****************************** Class methods ********************************/
/*****************************************************************************/
/*****************************************************************************/

IrregTiledRTCollection::IrregTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int border, 
    BGMasker* bgm) : TiledRTCollection(name, refDDRName, tilesPath) {

    this->border = border;
    this->bgm = bgm;
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
        int32_t osrMaxLevel = -1;
        cv::Mat mat;
        if (isSvs) {
            osr = openslide_open(initialPaths[i].c_str());
            osrMaxLevel = getLargestLevel(osr);
            openslide_get_level_dimensions(osr, osrMaxLevel, &w, &h);
            cv::Rect_<int64_t> roi(0, 0, w, h);
            osrRegionToCVMat(osr, roi, osrMaxLevel, mat);
        } else {
            mat = cv::imread(initialPaths[i]);
            h = mat.rows;
            w = mat.cols;
        }

        // Performs the threshold analysis and then tile the image
        cv::Mat thMask = bgm->bgMask(mat);
        std::list<cv::Rect_<int64_t> > tiles = autoTiler(
            thMask, this->border, &mat);

        // Actually tile the image given the list of ROIs
        int drId=0;
        for (cv::Rect_<int64_t> tile : tiles) {
            std::string path = tilesPath + "/" + name + "/";
            path += "t" + to_string(drId) + TILE_EXT;
            cv::imwrite(path, mat(tile));
            
            // Create new RT tile from roi
            std::string drName = "t" + to_string(drId);
            DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
            ddr2d->setName(drName);
            ddr2d->setId(refDDRName);
            ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
            ddr2d->setIsAppInput(true);
            ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
            ddr2d->setInputFileName(path);
            RegionTemplate* newRT = new RegionTemplate();
            newRT->insertDataRegion(ddr2d);
            newRT->setName(name);

            // Add the tile and the RT to the internal containers
            rois.push_back(tile);
            rts.push_back(
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
