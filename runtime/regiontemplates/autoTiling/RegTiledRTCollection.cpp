#include "RegTiledRTCollection.h"

/*****************************************************************************/
/***************************** Helper functions ******************************/
/*****************************************************************************/

void createTile(cv::Rect_<int64_t> roi, std::string tilesPath, 
    std::string name, int drId, std::string refDDRName, 
    std::vector<std::pair<std::string, RegionTemplate*> >& rts) {
    
    // Create new RT tile from roi
    std::string drName = "t" + to_string(drId);
    DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
    ddr2d->setRoi(roi);
    ddr2d->setSvs();
    ddr2d->setName(drName);
    ddr2d->setId(refDDRName);
    ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
    ddr2d->setIsAppInput(true);
    ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
    ddr2d->setInputFileName(tilesPath);
    RegionTemplate* newRT = new RegionTemplate();
    newRT->insertDataRegion(ddr2d);
    newRT->setName(name);

    rts.push_back(
        std::pair<std::string, RegionTemplate*>(drName, newRT));
}

inline int cost(const cv::Mat& img, const cv::Rect_<int64_t>& r) {
    return cv::sum(img(cv::Range(r.y, r.y+r.height), 
                       cv::Range(r.x, r.x+r.width)))[0];
}

void stddev(std::list<cv::Rect_<int64_t>> rs, 
    const cv::Mat& img, std::string name) {

    float mean = 0;
    for (cv::Rect_<int64_t> r : rs)
        mean += cost(img, r);
    mean /= rs.size();

    float var = 0;
    for (cv::Rect_<int64_t> r : rs)
        var += pow(cost(img, r)-mean, 2);
    
    std::cout << "[PROFILING][STDDEV][" << name << "]" 
        << (sqrt(var/(rs.size()-1))) << std::endl;
}

/*****************************************************************************/
/****************************** Class methods ********************************/
/*****************************************************************************/

RegTiledRTCollection::RegTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int64_t tw, int64_t th, 
    int64_t border) : TiledRTCollection(name, refDDRName, tilesPath) {

    if (border > tw || border > th) {
        std::cout << "Border cannot be greater than a tile's width" 
            << std::endl;
        exit(-2);
    }

    this->border = border;
    this->tw = tw;
    this->th = th;
    this->nTiles = 0;
}

RegTiledRTCollection::RegTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int64_t nTiles, 
    int64_t border) : TiledRTCollection(name, refDDRName, tilesPath) {

    this->border = border;
    this->nTiles = nTiles;
}

void RegTiledRTCollection::customTiling() {
    // Go through all images
    for (int i=0; i<initialPaths.size(); i++) {
        // Open image for tiling
        int64_t w = -1;
        int64_t h = -1;
        openslide_t* osr;
        int32_t osrMinLevel = -1;
        int32_t osrMaxLevel = 0; // svs standard: max level = 0
        float ratiow;
        float ratioh; 
        cv::Mat mat;

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
        osrRegionToCVMat(osr, roi, osrMinLevel, mat);

        // Calculates the ratio between largest and smallest 
        // images' dimensions for later conversion
        ratiow /= w;
        ratioh /= h;

        // Create the list of tiles for the current image
        std::list<cv::Rect_<int64_t>> rois;

        // Calculates the tiles sizes given the nTiles
        float k;
        if (this->nTiles > 1) {
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
            k = sqrt((float)this->nTiles*(float)h/(float)w);

            // ... Actually, since nx and ny are floats, we won't use this 
            // ... equations below.
            // // As such, we can calculate the tiles sizes
            // this->tw = min(floor((float)k*(float)w/(float)this->nTiles), 
            //     (float)w);
            // this->th = min(floor((float)h/k), (float)h);

            // std::cout << "w: " << w << ", h: " << h << std::endl;
            // std::cout << "k = " << k << ", sizes = " << this->tw 
            //     << ", " << this->th << std::endl;
            // std::cout << "nx: " << k << ", ny: " << nTiles/k << std::endl;
        } else {
            std::list<cv::Rect_<int64_t>> rois;
            cv::Rect_<int64_t> r;
            r.x = 0;
            r.width = ratiow*w;
            r.y = 0;
            r.height = ratioh*h;
            
            createTile(r, this->tilesPath, this->name, 0,
                this->refDDRName, this->rts);

            this->tiles.push_back(rois);
            return;
        }

        // Determine the number of tile levels to be had by getting the best
        // possible approximation. Since nx and ny are floats, we must first
        // round them. There are four rounding possibilities (the combinations 
        // of floor and ceil for each coord). Each one is tested to find the 
        // combination which results in the closest number of resulting tiles
        // to nTiles. If there are two possibilities with the same number of 
        // tiles, the one with the smallest perimeter is chosen.

        int xTiles;
        int yTiles;
        int curNt;
        int bestPrm;

        // Get the info for the first combination
        int xTilesTmp = floor(k);
        int yTilesTmp = floor(nTiles/k);
        int curNtTmp = xTilesTmp * yTilesTmp;
        long curPrm = 2*xTilesTmp*h + 2*yTilesTmp*w;

        // Set the first combination as the first best
        xTiles = xTilesTmp;
        yTiles = yTilesTmp;
        curNt = curNtTmp;
        bestPrm = curPrm;

        // Check if the second combination is better
        xTilesTmp = ceil(k);
        yTilesTmp = floor(nTiles/k);
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
        xTilesTmp = floor(k);
        yTilesTmp = ceil(nTiles/k);
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
        xTilesTmp = ceil(k);
        yTilesTmp = ceil(nTiles/k);
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
        this->tw = floor(w/xTiles);
        this->th = floor(h/yTiles);

#ifdef DEBUG
        std::cout << "Full size:" << w << "x" << h << std::endl;
        std::cout << "xTiles:" << xTiles << std::endl;
        std::cout << "yTiles:" << yTiles << std::endl;
#endif

        // Check whether the border can generate an Out of Bounds (OoB)
        if (this->border >= this->tw || this->border >= this->th) {
            std::cout << "[RegTiledRTCollection] border must be less than "
                      << "the size of the tile: tw=" << this->tw
                      << ", th=" << this->th << ", border" 
                      << this->border << std::endl;
            exit(-1);
        }

        // Create regular tiles, except the last line and column
        for (int ti=0; ti<yTiles; ti++) {
            for (int tj=0; tj<xTiles; tj++) {
                // Set the x and y rect coordinates, checking for OoB
                int tjTmp = tj==0? 0 : tj*this->tw-this->border;
                int tiTmp = ti==0? 0 : ti*this->th-this->border;

                // Set the width and height no OoB check is required since
                // this->border must be less than this->tw and this->th
                int tjjTmp = tj==(yTiles-1)? w-tjTmp : this->tw+this->border;
                int tiiTmp = ti==(xTiles-1)? h-tiTmp : this->th+this->border;
                
                // Create the roi for the current tile
                cv::Rect_<int64_t> roi(
                    tjTmp, tiTmp, tjjTmp, tiiTmp);
                rois.push_back(roi);
#ifdef DEBUG
                std::cout << "creating regular roi " << roi.x << "+" 
                          << roi.width << "x" << roi.y << "+" 
                          << roi.height << std::endl;
#endif
            }
        }

// #ifdef PROFILING
//         // Gets std-dev of tiles' sizes
//         stddev(rois, mat, "ALL");
// #endif

        // Creates the actual tiles with the correct size
        int drId = 0;
        std::list<cv::Rect_<int64_t>> newRois;
        for (cv::Rect_<int64_t> r : rois) {
            cv::rectangle(mat, cv::Point(r.x,r.y),
                cv::Point(r.x+r.width,r.y+r.height),
                (0,0,0),3);
            r.x *= ratiow;
            r.width *= ratiow;
            r.y *= ratioh;
            r.height *= ratioh;

            newRois.push_back(r);
            createTile(r, this->tilesPath, this->name, drId++,
                this->refDDRName, this->rts);
        }

#ifdef DEBUG
        cv::imwrite("./maskf.png", mat);
#endif

        // Close .svs file
        openslide_close(osr);

        // Add the current image tiles to the tiles vector
        this->tiles.push_back(newRois);
    }
}
