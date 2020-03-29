#include "fixedGrid.h"

int fixedGrid(int64_t nTiles, int64_t w, int64_t h, int64_t mw, int64_t mh, 
        std::list<cv::Rect_<int64_t>>& rois) {
    // Finds the best fit for square tiles that match nTiles by the 
    // equation:
    //    nTiles = nx(number of divisions on x) * ny(same on y)
    //    e.g., 6 tiles organized as a 2x3 grid has nx=2 and ny=3
    // Assuming that nx = k and ny = nTiles/k (arbitrary k with 
    // 1<=k<=nTiles) we must find k that minimizes the sum of all 
    // perimeters of the generated tiles, thus making them square.
    // Being Sc(sum of perimeters) = 2(tw+th)*(nx*ny), and taking 
    // the first derivative Sc'=0 we have that the value of k for 
    // Sc minimal is:
    float k = sqrt((float)nTiles*(float)h/(float)w);

    // Determine the number of tile levels to be had by getting the best
    // possible approximation. Since nx and ny are floats, we must first
    // round them. There are four rounding possibilities (the combinations 
    // of floor and ceil for each coord). Each one is tested to find the 
    // combination which results in the closest number of resulting tiles
    // to nTiles. If there are two possibilities with the same number of 
    // tiles, the one with the smallest perimeter is chosen.

    int64_t curNt;
    int64_t bestPrm;

    // Get the info for the first combination
    int64_t xTiles;
    int64_t yTiles;
    int64_t xTilesTmp = floor(nTiles/k);
    int64_t yTilesTmp = floor(k);
    int64_t curNtTmp = xTilesTmp * yTilesTmp;
    int64_t curPrm = 2*xTilesTmp*h + 2*yTilesTmp*w;

    // Set the first combination as the first best
    xTiles = xTilesTmp;
    yTiles = yTilesTmp;
    curNt = curNtTmp;
    bestPrm = curPrm;

    // Check if the second combination is better
    xTilesTmp = floor(nTiles/k);
    yTilesTmp = ceil(k);
    curNtTmp = xTilesTmp * yTilesTmp;
    curPrm = 2*xTilesTmp*h + 2*yTilesTmp*w;
    // If the number of tiles is better or if it is the same as the best, 
    // but has better perimeter (less)
    if (abs(curNtTmp-nTiles) < abs(curNt-nTiles) ||
        (abs(curNtTmp-nTiles) == abs(curNt-nTiles) && curPrm < bestPrm) ) {
        xTiles = xTilesTmp;
        yTiles = yTilesTmp;
        curNt = curNtTmp;
        bestPrm = curPrm;
    }

    // Check if the third combination is better
    xTilesTmp = ceil(nTiles/k);
    yTilesTmp = floor(k);
    curNtTmp = xTilesTmp * yTilesTmp;
    curPrm = 2*xTilesTmp*h + 2*yTilesTmp*w;
    // If the number of tiles is better or if it is the same as the best, 
    // but has better perimeter (less)
    if (abs(curNtTmp-nTiles) < abs(curNt-nTiles) ||
        (abs(curNtTmp-nTiles) == abs(curNt-nTiles) && curPrm < bestPrm) ) {
        xTiles = xTilesTmp;
        yTiles = yTilesTmp;
        curNt = curNtTmp;
        bestPrm = curPrm;
    }

    // Check if the fourth combination is better
    xTilesTmp = ceil(nTiles/k);
    yTilesTmp = ceil(k);
    curNtTmp = xTilesTmp * yTilesTmp;
    curPrm = 2*xTilesTmp*h + 2*yTilesTmp*w;
    // If the number of tiles is better or if it is the same as the best, 
    // but has better perimeter (less)
    if (abs(curNtTmp-nTiles) < abs(curNt-nTiles) ||
        (abs(curNtTmp-nTiles) == abs(curNt-nTiles) && curPrm < bestPrm) ) {
        xTiles = xTilesTmp;
        yTiles = yTilesTmp;
        curNt = curNtTmp;
        bestPrm = curPrm;
    }

    // Updates the tile sizes for the best configuration
    int64_t tw = floor(w/xTiles);
    int64_t th = floor(h/yTiles);

    #ifdef DEBUG
    std::cout << "Full size:" << w << "x" << h << std::endl;
    std::cout << "xTiles:" << xTiles << std::endl;
    std::cout << "yTiles:" << yTiles << std::endl;
    std::cout << "tw:" << tw << std::endl;
    std::cout << "th:" << th << std::endl;
    #endif

    // Create regular tiles, except the last line and column
    for (int ti=0; ti<yTiles; ti++) {
        for (int tj=0; tj<xTiles; tj++) {
            // Set the x and y rect coordinates
            int tjTmp  = mw + tj*tw;
            int tjjTmp = mw + tj==(xTiles-1)? w-(tj*tw) : tw;
            int tiTmp  = mh + ti*th;
            int tiiTmp = mh + ti==(yTiles-1)? h-(ti*th) : th;

            // Create the roi for the current tile
            cv::Rect_<int64_t> roi(
                tjTmp, tiTmp, tjjTmp, tiiTmp);
            rois.push_back(roi);

            // #ifdef DEBUG
            std::cout << "creating regular roi " << roi.x << "+" 
                      << roi.width << "x" << roi.y << "+" 
                      << roi.height << std::endl;
            // #endif
        }
    }
}