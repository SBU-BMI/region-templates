#include "RegTiledRTCollection.h"

/*****************************************************************************/
/***************************** Helper functions ******************************/
/*****************************************************************************/

void createTile(bool isSvs, cv:: Rect_<int64_t> roi, openslide_t* osr,
    int32_t osrMaxLevel, cv::Mat mat, std::string tilesPath, std::string name,
    int i, int ti, int tj, int drId, std::string refDDRName, 
    std::vector<std::pair<std::string, RegionTemplate*> >& rts) {

    cv::Mat tile;
    if (isSvs) {
        osrRegionToCVMat(osr, roi, osrMaxLevel, tile);
    } else {
        tile = mat(roi);
    }
    std::string path = tilesPath + "/" + name + "/";
    path += "/i" + to_string(i);
    path += "ti" + to_string(ti);
    path += "tj" + to_string(tj) + TILE_EXT;
    cv::imwrite(path, tile);
    
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

    rts.push_back(
        std::pair<std::string, RegionTemplate*>(drName, newRT));

    // // Close .svs file
    // if (isSvs) {
    //     openslide_close(osr);
    // }
}

/*****************************************************************************/
/****************************** Class methods ********************************/
/*****************************************************************************/

RegTiledRTCollection::RegTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int64_t tw, 
    int64_t th, int border) : TiledRTCollection(name, refDDRName, tilesPath) {

    if (border > tw || border > th) {
        std::cout << "Border cannot be greater than a tile width" << std::endl;
        exit(-2);
    }

    this->border = border;
    this->tw = tw;
    this->th = th;
}

void RegTiledRTCollection::customTiling() {
    std::string drName;
    // Go through all images
    for (int i=0; i<initialPaths.size(); i++) {
        bool isSvs = isSVS(initialPaths[i]);

        // Open image for tiling
        int64_t w = -1;
        int64_t h = -1;
        openslide_t* osr;
        int32_t osrMaxLevel = 0; // svs standard: max level = 0
        cv::Mat mat;
        if (isSvs) {
            osr = openslide_open(initialPaths[i].c_str());
            openslide_get_level0_dimensions(osr, &w, &h);
        } else {
            mat = cv::imread(initialPaths[i]);
            h = mat.rows;
            w = mat.cols;
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
                createTile(isSvs, roi, osr, osrMaxLevel, mat, 
                    this->tilesPath, this->name, i, ti, tj, drId++,
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
            createTile(isSvs, roi, osr, osrMaxLevel, mat, 
                this->tilesPath, this->name, i, 0, tj, drId++,
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
                createTile(isSvs, roi, osr, osrMaxLevel, mat, 
                    this->tilesPath, this->name, i, ti, tj, drId++,
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
            createTile(isSvs, roi, osr, osrMaxLevel, mat, 
                this->tilesPath, this->name, i, ti, 0, drId++,
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
                createTile(isSvs, roi, osr, osrMaxLevel, mat, 
                    this->tilesPath, this->name, i, ti, tj, drId++,
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
            createTile(isSvs, roi, osr, osrMaxLevel, mat, 
                this->tilesPath, this->name, i, ti, tj, drId++,
                this->refDDRName, this->rts);
        }

        // Add the current image tiles to the tiles vector
        this->tiles.push_back(rois);

        // // Close .svs file
        if (isSvs) {
            openslide_close(osr);
        }
    }
}
