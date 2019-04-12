#include "IrregTiledRTCollection.h"

/*****************************************************************************/
/***************************** Helper functions ******************************/
/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************/
/**                             Inside Overlap                              **/
/*****************************************************************************/

inline bool isInside(int x, int y, cv::Rect_<int64_t> r2) {
    int64_t xf = r2.x + r2.width;
    int64_t yf = r2.y + r2.height;
    return r2.x <= x && r2.y <= y && xf >= x && yf >= y;
}

inline bool isInside(cv::Rect_<int64_t> r1, cv::Rect_<int64_t> r2) {
    int64_t xf = r1.x + r1.width;
    int64_t yf = r1.y + r1.height;
    return isInside(r1.x, r1.y, r2) && isInside(xf, yf, r2);
}

void removeInsideOvlp(std::list<cv::Rect_<int64_t> >& output) {
    // first create an array for parallelization
    int outS = output.size();
    cv::Rect_<int64_t> outArray[outS];
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

inline bool hOvlp(cv::Rect_<int64_t> big, cv::Rect_<int64_t> small) {
    int64_t bxo = big.x + big.width;
    int64_t byo = big.y + big.height;
    int64_t sxo = small.x + small.width;
    int64_t syo = small.y + small.height;

    return big.y <= small.y && byo >= syo 
        && ((big.x < small.x && bxo > small.x)
        || (big.x < sxo && bxo > sxo));
}

inline bool vOvlp(cv::Rect_<int64_t> big, cv::Rect_<int64_t> small) {
    int64_t bxo = big.x + big.width;
    int64_t byo = big.y + big.height;
    int64_t sxo = small.x + small.width;
    int64_t syo = small.y + small.height;

    return big.x <= small.x && bxo >= sxo 
        && ((big.y < small.y && byo > small.y)
        || (big.y < syo && byo > syo));
}

void removeSideOvlp(std::list<cv::Rect_<int64_t> >& output) {
    // first create an array for parallelization
    int outS = output.size();
    cv::Rect_<int64_t> outArray[outS];
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
                if (outArray[i].y > outArray[j].y) {
                    big = j; small = i;
                }

                // remove the overlapping of small with big
                if (outArray[small].x > outArray[big].x) // big left
                    outArray[small].x = outArray[big].x + outArray[big].width;
                else // big on the right
                    outArray[small].width = outArray[big].x - outArray[small].x;
                    // outArray[small].xo = outArray[big].x;
            }
            // check if there is a vertical overlapping
            if (vOvlp(outArray[i], outArray[j]) 
                || vOvlp(outArray[i], outArray[j])) {

                // find which is the big one
                big = i; small = j;
                if (outArray[i].x > outArray[j].x) {
                    big = j; small = i;
                }

                // remove the overlapping of small with big
                if (outArray[small].y > outArray[big].y) // big up
                    outArray[small].y = outArray[big].y + outArray[big].height;
                else // big is the down region
                    outArray[small].width = outArray[big].x - outArray[small].x;
                    // outArray[small].yo = outArray[big].y;
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

inline int area (cv::Rect_<int64_t> r) {
    return r.width * r.height;
}

void removeDiagOvlp(std::list<cv::Rect_<int64_t> >& ovlpCand, 
    std::list<cv::Rect_<int64_t> >& nonMod) {

    // first create an array for parallelization
    int outS = ovlpCand.size();
    cv::Rect_<int64_t> outArray[outS];
    std::copy(ovlpCand.begin(), ovlpCand.end(), outArray);

    // array for the replaced 3 new blocks for every initial block
    cv::Rect_<int64_t> outRepArray[outS][3];

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
            cv::Rect_<int64_t> b = outArray[i];
            cv::Rect_<int64_t> s = outArray[j];

            // break the big block if there is a diagonal overlap
            if (b.x > s.x && b.x < s.x+s.width 
                && b.y+b.height > s.y && b.y+b.height < s.y+s.height) {

                // big on upper right diagonal
                // only break down if <i> is the big block of a diagonal overlap
                if (area(outArray[i]) < area(outArray[j])) {
                    outRepArrayS[i] = true;
                    continue;
                }
                outRepArrayR[i] = true;
                outRepArray[i][0] = cv::Rect_<int64_t>(
                    b.x, b.y, s.x+s.width-b.x, s.y-b.y);
                outRepArray[i][1] = cv::Rect_<int64_t>(
                    s.x+s.width, b.y, b.x+b.width-s.x-s.width, 
                    b.y+b.height-s.y);
                outRepArray[i][2] = cv::Rect_<int64_t>(
                    s.x+s.width, s.y, b.x+b.width-s.x-s.width, 
                    b.y+b.height-s.y-s.width);

                // outRepArray[i][0] = {b.xi, b.yi, s.xo, s.yi};
                // outRepArray[i][1] = {s.xo, b.y, b.xo, s.y};
                // outRepArray[i][2] = {s.xo, s.y, b.xo, b.yo};
            } else if (b.x+b.width > s.x && b.x+b.width < s.x+s.width 
                && b.y+b.height > s.y && b.y+b.height < s.y+s.height) {

                // big on upper left
                // only break down if <i> is the big block of a diagonal overlap
                if (area(outArray[i]) < area(outArray[j])) {
                    outRepArrayS[i] = true;
                    continue;
                }
                outRepArrayR[i] = true;
                outRepArray[i][0] = cv::Rect_<int64_t>(
                    b.x, b.y,s.x-b.x, s.y-b.y);
                outRepArray[i][1] = cv::Rect_<int64_t>(
                    s.x, b.y, b.x+b.width-s.x, s.y-b.y);
                outRepArray[i][2] = cv::Rect_<int64_t>(
                    b.x, s.y, s.x-b.x, s.y-b.y);

                // outRepArray[i][0] = {b.x, b.y, s.x, s.y};
                // outRepArray[i][1] = {s.x, b.y, b.xo, s.y};
                // outRepArray[i][2] = {b.x, s.y, s.x, b.yo};
            } else if (b.x > s.x && b.x < s.x+s.width 
                && b.y > s.y && b.y < s.y+s.height) {

                // big on bottom right
                // only break down if <i> is the big block of a diagonal overlap
                if (area(outArray[i]) < area(outArray[j])) {
                    outRepArrayS[i] = true;
                    continue;
                }
                outRepArrayR[i] = true;
                outRepArray[i][0] = cv::Rect_<int64_t>(
                    s.x+s.width, b.y, b.x+b.width-s.x, s.y+s.height-b.y);
                outRepArray[i][1] = cv::Rect_<int64_t>(
                    b.x, s.y+s.height, s.y+s.width-b.x, 
                    b.y+b.height-s.y-s.height);
                outRepArray[i][2] = cv::Rect_<int64_t>(
                    s.x+s.width, s.y+s.height, b.x+b.width-s.x-s.width, 
                    b.y+b.height-s.y-s.height);

                // outRepArray[i][0] = {s.xo, b.y, b.xo, s.yo};
                // outRepArray[i][1] = {b.x, s.yo, s.xo, b.yo};
                // outRepArray[i][2] = {s.xo, s.yo, b.xo, b.yo};
            } else if (b.x+b.width > s.x && b.x+b.width < s.x+s.width 
                && b.y > s.y && b.y < s.y+s.height) {

                // big on bottom left
                // only break down if <i> is the big block of a diagonal overlap
                if (area(outArray[i]) < area(outArray[j])) {
                    outRepArrayS[i] = true;
                    continue;
                }
                outRepArrayR[i] = true;
                outRepArray[i][0] = cv::Rect_<int64_t>(
                    b.x, b.y, s.x-b.x-b.width, s.y+s.width-b.y);
                outRepArray[i][1] = cv::Rect_<int64_t>(
                    s.x, s.y+s.height, s.x-b.x-b.width, 
                    b.y+b.height-s.y-s.height);
                outRepArray[i][2] = cv::Rect_<int64_t>(
                    b.x, s.y+s.height, b.x+b.width-s.x, 
                    b.y+b.height-s.y-s.height);

                // outRepArray[i][0] = {b.x, b.y, s.x, s.yo};
                // outRepArray[i][1] = {s.x, s.yo, b.xo, b.yo};
                // outRepArray[i][2] = {b.x, s.yo, s.x, b.yo};
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
/*****************************************************************************/

// yi = oldY
// yo = min(r,cur)
// xo = width
void newBlocks(std::list<cv::Rect_<int64_t>>& out, 
    std::multiset<cv::Rect_<int64_t>,rect_tCompX> cur, 
    int yi, int yh, int xw) {
    // int yi, int yo, int xo) {

    int xi = 0;
    for (cv::Rect_<int64_t> r : cur) {
        cv::Rect_<int64_t> newR(xi, yi, r.xi, yo);
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
    std::list<rect_t>& output, int maxCols, cv::Mat& input) {

    int oldY = 0;
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

/*****************************************************************************/
/**                             Full AutoTiler                              **/
/*****************************************************************************/

std::list<cv::Rect_<int64_t> > autoTiler(cv::Mat& input, int border, 
    int bgThreshold, int erosionSize) {

    std::list<cv::Rect_<int64_t> > output;

    // merge regions of interest together and mark them separately
    cv::Mat stats, centroids;
    cv::connectedComponentsWithStats(input, input, stats, centroids);

    double maxLabel;
    cv::minMaxLoc(input, NULL, &maxLabel);
    std::cout << "[autoTiler] Image size: " 
        << input.cols << "x" << input.rows << std::endl;
    std::cout << "[autoTiler] Initial dense regions: " << maxLabel << std::endl;
    
    // generate the list of dense areas
    std::list<cv::Rect_<int64_t> > ovlpCand;
    for (int i=1; i<=maxLabel; i++) { // i=1 ignores background
        int xi = stats.at<int>(i, cv::CC_STAT_LEFT);
        int yi = stats.at<int>(i, cv::CC_STAT_TOP);
        int xw = stats.at<int>(i, cv::CC_STAT_WIDTH);
        int yh = stats.at<int>(i, cv::CC_STAT_HEIGHT);
        cv::Rect_<int64_t> rr(xi, yi, xw, yh);
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
    std::list<cv::Rect_<int64_t> > dense(output);
    
    // generate the background regions
    generateBackground(dense, output, input.cols, input);

    std::cout << "[autoTiler] Total regions to process: " 
        << output.size() << std::endl;
    cv::Mat final = input.clone();
        
    // add a border to all rect regions
    for (std::list<rect_t>::iterator r=output.begin(); r!=output.end(); r++) {
        r->xi = std::max(r->xi-border, 0);
        r->xo = std::min(r->xo+border, input.cols);
        r->yi = std::max(r->yi-border, 0);
        r->yo = std::min(r->yo+border, input.rows);

        // draw areas for verification
        cv::rectangle(final, cv::Point(r->xi,r->yi), 
            cv::Point(r->xo,r->yo),(0,0,0),3);
    }

    return output;
}

/*****************************************************************************/
/****************************** Class methods ********************************/
/*****************************************************************************/
/*****************************************************************************/

RegTiledRTCollection::RegTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int64_t tw, 
    int64_t th) : TiledRTCollection(name, refDDRName, tilesPath) {

    this->tw = tw;
    this->th = th;
}

void RegTiledRTCollection::customTiling() {
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

        std::List<Rect_<int64_t> > tiles = autoTiler(mat);

        int i=0;
        for (Rect_<int64_t> tile : tiles) {
            std::string path = tilesPath + "/" + name + "/";
            path += "t" + to_string(i);
            cv::imwrite(path, mat(tile));
            
            // Create new RT tile from roi
            std::string drName = "t" + to_string(i);
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
            i++;
        }

        // Close .svs file
        if (isSvs) {
            openslide_close(osr);
        }

        // Add the current image tiles to the tiles vector
        this->tiles.push_back(rois);
    }
}
