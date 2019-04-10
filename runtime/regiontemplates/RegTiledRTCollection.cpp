#include "RegTiledRTCollection.h"

/*****************************************************************************/
/***************************** Helper functions ******************************/
/*****************************************************************************/


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

        // Create regular tiles
        for (int ti=0; ti<yTiles; ti++) {
            for (int tj=0; tj<xTiles; tj++) {
                // Create the roi for the current tile
                cv::Rect_<int64_t> roi(tj*this->tw, 
                    ti*this->th, this->tw, this->th);
                rois.push_back(roi);

                // Create a tile file
                cv::Mat tile;
                if (isSvs) {
                    osrRegionToCVMat(osr, roi, osrMaxLevel, tile);
                } else {
                    tile = mat(roi);
                }
                std::string path = this->tilesPath + "/" + name + "/";
                path += "/i" + to_string(i);
                path += "ti" + to_string(ti);
                path += "tj" + to_string(tj) + TILE_EXT;
                cv::imwrite(path, tile);
                
                // Create new RT tile from roi
                DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
                ddr2d->setName(refDDRName);
                ddr2d->setId(refDDRName);
                ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
                ddr2d->setIsAppInput(true);
                ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
                ddr2d->setInputFileName(path);
                RegionTemplate* newRT = new RegionTemplate();
                newRT->insertDataRegion(ddr2d);
                newRT->setName(this->name);

                this->rts.push_back(newRT);

                // Close .svs file
                if (isSvs) {
                    openslide_close(osr);
                }
            }
        }

        // Add the current image tiles to the tiles vector
        this->tiles.push_back(rois);




    }
}
