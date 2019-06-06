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
    
    std::cout << "[PROFILING][" << name << "]" 
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
        int32_t osrMaxLevel = 0; // svs standard: max level = 0
        cv::Mat mat;
        osr = openslide_open(initialPaths[i].c_str());
        openslide_get_level0_dimensions(osr, &w, &h);
        cv::Rect_<int64_t> roi(0, 0, w, h);
        osrRegionToCVMat(osr, roi, 0, mat);

        // Calculates the tiles sizes given the nTiles
        if (this->nTiles != 0) {
            // Finds the best fit for square tiles that match nTiles by the 
            // equation:
            //    nTiles = nx(number of divisions on x) * ny(same on y)
            //    e.g., 6 tiles organized as a 2x3 grid has nx=2 and ny=3
            // Assuming that nx = k and ny = nTiles/k (arbitrary k with 
            // 1<=k<=nTiles) we must find k that minimizes the sum of all 
            // circumferences of the generated tiles, thus making them square.
            // Being Sc(sum of circumferences) = 2(tw+th)*(nx*ny), and taking 
            // the first derivative Sc'=0 we have that the value of k for 
            // Sc minimal is:
            float k = sqrt((float)this->nTiles*(float)h/(float)w);

            // As such, we can calculate the tiles sizes
            this->tw = min(floor((float)k*(float)w/(float)this->nTiles), 
                (float)w);
            this->th = min(floor((float)h/k), (float)h);
            // std::cout << "w: " << w << ", h: " << h << std::endl;
            // std::cout << "k = " << k << ", sizes = " << this->tw 
            //     << ", " << this->th << std::endl;
            // std::cout << "nx: " << k << ", ny: " << nTiles/k << std::endl;
        }

        // Determine the number of tile levels to be had
        int xTiles = floor(w/this->tw);
        int yTiles = floor(h/this->th);

        // Create the list of tiles for the current image
        std::list<cv::Rect_<int64_t>> rois;

#ifdef DEBUG
        std::cout << "Full size:" << w << "x" << h << std::endl;
#endif

        // Create regular tiles
        int drId = 0;
        for (int ti=0; ti<yTiles; ti++) {
            for (int tj=0; tj<xTiles; tj++) {
                // Create the roi for the current tile
                int tjTmp = tj==0? 0 : tj*this->tw-this->border;
                int tiTmp = ti==0? 0 : ti*this->th-this->border;
                cv::Rect_<int64_t> roi(
                    tjTmp, tiTmp, 
                    this->tw+this->border, this->th+this->border);
                rois.push_back(roi);

#ifdef DEBUG
                std::cout << "creating roi " << roi.x << "+" << roi.width 
                    << "x" << roi.y << "+" << roi.height << std::endl;
#endif

                // Create a tile file
                createTile(roi, this->tilesPath, this->name, drId++,
                    this->refDDRName, this->rts);
            }
        }

        // Create irregular border tiles for the last vertical column
        if ((float)w/this->tw > xTiles) {
            int tj = xTiles*this->tw;

            // Create first tile
            cv::Rect_<int64_t> roi(
                tj-this->border, 0, 
                w-tj, this->th+this->border);
            rois.push_back(roi);

#ifdef DEBUG
            std::cout << "creating roi " << roi.x << "+" << roi.width 
                    << "x" << roi.y << "+" << roi.height << std::endl;
#endif

            // Create the first tile file
            createTile(roi, this->tilesPath, this->name, drId++,
                this->refDDRName, this->rts);

            for (int ti=1; ti<yTiles; ti++) {
                // Create the roi for the current tile
                cv::Rect_<int64_t> roi(
                    tj-this->border, ti*this->th-this->border, 
                    w-tj, this->th+this->border);
                rois.push_back(roi);

#ifdef DEBUG
                std::cout << "creating roi " << roi.x << "+" << roi.width 
                    << "x" << roi.y << "+" << roi.height << std::endl;
#endif

                // Create a tile file
                createTile(roi, this->tilesPath, this->name, drId++,
                    this->refDDRName, this->rts);
            }
        }
        // Create irregular border tiles for the last horizontal line
        if ((float)h/this->th > yTiles) {
            int ti = yTiles*this->th;

            // Create the roi for the first tile
            cv::Rect_<int64_t> roi(
                0, ti-this->border, 
                this->tw+this->border, h-ti);
            rois.push_back(roi);

#ifdef DEBUG
            std::cout << "creating roi " << roi.x << "+" << roi.width 
                << "x" << roi.y << "+" << roi.height << std::endl;
#endif

            // Create the first tile file
            createTile(roi, this->tilesPath, this->name, drId++,
                    this->refDDRName, this->rts);

            for (int tj=1; tj<xTiles; tj++) {
                // Create the roi for the current tile
                cv::Rect_<int64_t> roi(
                    tj*this->tw-this->border, ti-this->border, 
                    this->tw+this->border, h-ti);
                rois.push_back(roi);

#ifdef DEBUG
                std::cout << "creating roi " << roi.x << "+" << roi.width 
                    << "x" << roi.y << "+" << roi.height << std::endl;
#endif

                // Create a tile file
                createTile(roi, this->tilesPath, this->name, drId++,
                    this->refDDRName, this->rts);
            }
        }

        // Create irregular border tile for the bottom right last tile
        if ((float)w/this->tw > xTiles && (float)h/this->th > yTiles) {
            int ti = yTiles*this->th;
            int tj = xTiles*this->tw;

            cv::Rect_<int64_t> roi(
                tj-this->border, ti-this->border, 
                w-tj, h-ti);
            rois.push_back(roi);

#ifdef DEBUG
            std::cout << "creating roi " << roi.x << "+" << roi.width 
                << "x" << roi.y << "+" << roi.height << std::endl;
#endif

            // Create a tile file
            createTile(roi, this->tilesPath, this->name, drId++,
                this->refDDRName, this->rts);
        }

        // Add the current image tiles to the tiles vector
        this->tiles.push_back(rois);

        // Close .svs file
        openslide_close(osr);

        // Gets std-dev of dense tiles' sizes
        stddev(rois, mat, "ALLSTDDEV");
    }
}
