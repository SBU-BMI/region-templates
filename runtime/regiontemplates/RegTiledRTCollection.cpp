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

    // Close .svs file
    if (isSvs) {
        openslide_close(osr);
    }
}

/*****************************************************************************/
/****************************** Class methods ********************************/
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
                cv::Rect_<int64_t> roi(tj*this->tw, 
                    ti*this->th, this->tw, this->th);
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
            for (int ti=0; ti<yTiles; ti++) {
                // Create the roi for the current tile
                cv::Rect_<int64_t> roi(tj, ti*this->th, w-tj, this->th);
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
            for (int tj=0; tj<xTiles; tj++) {
                // Create the roi for the current tile
                cv::Rect_<int64_t> roi(tj*this->tw, ti, this->tw, h-ti);
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

            cv::Rect_<int64_t> roi(tj, ti, w-tj, h-ti);
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
    }
}
