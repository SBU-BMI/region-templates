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
    int64_t borders, CostFunction* cfunc) 
        : TiledRTCollection(name, refDDRName, tilesPath, borders, cfunc) {

    if (this->borders > tw || this->borders > th) {
        std::cout << "Border cannot be greater than a tile's width" 
            << std::endl;
        exit(-2);
    }

    this->tw = tw;
    this->th = th;
    this->nTiles = 0;
}

RegTiledRTCollection::RegTiledRTCollection(std::string name, 
    std::string refDDRName, std::string tilesPath, int64_t nTiles, 
    int64_t borders, CostFunction* cfunc) 
        : TiledRTCollection(name, refDDRName, tilesPath, borders, cfunc) {

    this->nTiles = nTiles;
}

void RegTiledRTCollection::customTiling() {
//     // Go through all images
//     for (int i=0; i<initialPaths.size(); i++) {
//         // Open image for tiling
//         int64_t w = -1;
//         int64_t h = -1;
//         openslide_t* osr;
//         int32_t osrMinLevel = -1;
//         int32_t osrMaxLevel = 0; // svs standard: max level = 0
//         float ratiow;
//         float ratioh; 
//         cv::Mat mat;

//         // Opens svs input file
//         osr = openslide_open(initialPaths[i].c_str());

//         // Gets info of largest image
//         openslide_get_level0_dimensions(osr, &w, &h);
//         ratiow = w;
//         ratioh = h;

//         // Opens smallest image as a cv mat
//         osrMinLevel = openslide_get_level_count(osr) - 1; // last level
//         openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
//         cv::Rect_<int64_t> roi(0, 0, w, h);
//         osrRegionToCVMat(osr, roi, osrMinLevel, mat);

//         // Calculates the ratio between largest and smallest 
//         // images' dimensions for later conversion
//         ratiow /= w;
//         ratioh /= h;

//         // Create the list of tiles for the current image
//         std::list<cv::Rect_<int64_t>> rois;

//         // Calculates the tiles sizes given the nTiles
//         if (this->nTiles <= 1) {
//             cv::Rect_<int64_t> r;
//             r.x = 0;
//             r.width = ratiow*w;
//             r.y = 0;
//             r.height = ratioh*h;
            
//             createTile(r, this->tilesPath, this->name, 0,
//                 this->refDDRName, this->rts);
//             rois.push_back(r);
//             this->tiles.push_back(rois);

//             // Close .svs file
//             openslide_close(osr);

//             return;
//         } else {
//             fixedGrid(this->nTiles, w, h, 0, 0, rois);
//         }

// // #ifdef PROFILING
// //         // Gets std-dev of tiles' sizes
// //         stddev(rois, mat, "ALL");
// // #endif

//         // Creates the actual tiles with the correct size
//         int drId = 0;
//         std::list<cv::Rect_<int64_t>> newRois;
//         for (cv::Rect_<int64_t> r : rois) {
//             cv::rectangle(mat, cv::Point(r.x,r.y),
//                 cv::Point(r.x+r.width,r.y+r.height),
//                 (0,0,0),3);
//             r.x *= ratiow;
//             r.width *= ratiow;
//             r.y *= ratioh;
//             r.height *= ratioh;

//             newRois.push_back(r);
//             createTile(r, this->tilesPath, this->name, drId++,
//                 this->refDDRName, this->rts);
//         }

// #ifdef DEBUG
//         cv::imwrite("./maskf.png", mat);
// #endif

//         // Close .svs file
//         openslide_close(osr);

//         // Add the current image tiles to the tiles vector
//         this->tiles.push_back(newRois);
//     }
}
