#include "svsUtils.h"

// Verifies whether the extension is .svs 
bool isSVS(std::string path) {
    std::size_t l = path.find_last_of(".");
    return path.substr(l).compare(".svs") == 0;
}

#define PROFILING_SVS

// Extracts a roi from osr described by r, outputting it into thisTile
void osrRegionToCVMat(openslide_t* osr, cv::Rect_<int64_t> r, 
    int level, cv::Mat& thisTile) {

    #ifdef PROFILING_SVS
    long t1 = Util::ClockGetTime();
    std::cout << "[osrRegionToCVMat] svs tile: " << r.height 
        << "x" << r.width << std::endl;
    #endif

    uint32_t* osrRegion = new uint32_t[r.width * r.height];
    openslide_read_region(osr, osrRegion, r.x, r.y, level, r.width, r.height);

    #ifdef PROFILING_SVS
    long t2 = Util::ClockGetTime();
    std::cout << "[osrRegionToCVMat] svs open: " << (t2-t1) << std::endl;
    #endif

    cv::Mat rgbaTile = cv::Mat(r.height, r.width, CV_8UC4, osrRegion);
    thisTile = cv::Mat(r.height, r.width, CV_8UC3, cv::Scalar(0, 0, 0));
    int64_t numOfPixelPerTile = thisTile.total();

    #ifdef PROFILING_SVS
    long t3 = Util::ClockGetTime();
    std::cout << "[osrRegionToCVMat] cv mat create: " << (t3-t2) << std::endl;
    #endif

    cv::cvtColor(rgbaTile, thisTile, CV_BGRA2RGB);

//    for (int64_t it = 0; it < numOfPixelPerTile; ++it) {
//        uint32_t p = osrRegion[it];
//
//        uint8_t a = (p >> 24) & 0xFF;
//        uint8_t r = (p >> 16) & 0xFF;
//        uint8_t g = (p >> 8) & 0xFF;
//        uint8_t b = p & 0xFF;
//
//        switch (a) {
//            case 0:
//                r = 0;
//                b = 0;
//                g = 0;
//                break;
//            case 255:
//                // no action needed
//                break;
//            default:
//                r = (r * 255 + a / 2) / a;
//                g = (g * 255 + a / 2) / a;
//                b = (b * 255 + a / 2) / a;
//                break;
//        }
//
//        // write back
//        thisTile.at<cv::Vec3b>(it)[0] = b;
//        thisTile.at<cv::Vec3b>(it)[1] = g;
//        thisTile.at<cv::Vec3b>(it)[2] = r;
//    }

    #ifdef PROFILING_SVS
    long t4 = Util::ClockGetTime();
    std::cout << "[osrRegionToCVMat] cv mat convert/update: " << (t4-t3) << std::endl;
    #endif

    delete[] osrRegion;

    #ifdef PROFILING_SVS
    long t5 = Util::ClockGetTime();
    std::cout << "[osrRegionToCVMat] tmp delete: " << (t5-t4) << std::endl;
    #endif

    return;
}

// default level is 0: largest image
// level=-1: min level
void osrFilenameToCVMat(std::string filename, cv::Mat& matOut, int level) {
    openslide_t* osr;
    int64_t w = -1;
    int64_t h = -1;

    // Opens svs input file
    osr = openslide_open(filename.c_str());

    if (level == -1)
        level = openslide_get_level_count(osr) - 1; // last level

    // Gets info of image
    openslide_get_level_dimensions(osr, level, &w, &h);

    // Gets the cv image from the osr format
    cv::Rect_<int64_t> roi(0, 0, w, h);
    osrRegionToCVMat(osr, roi, 0, matOut);
}
