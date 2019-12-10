#include "svsUtils.h"

// Verifies whether the extension is .svs 
bool isSVS(std::string path) {
    std::size_t l = path.find_last_of(".");
    return path.substr(l).compare(".svs") == 0;
}

// Extracts a roi from osr described by r, outputting it into thisTile
void osrRegionToCVMat(openslide_t* osr, cv::Rect_<int64_t> r, 
    int level, cv::Mat& thisTile) {

    uint32_t* osrRegion = new uint32_t[r.width * r.height];
    openslide_read_region(osr, osrRegion, r.x, r.y, level, r.width, r.height);

    thisTile = cv::Mat(r.height, r.width, CV_8UC3, cv::Scalar(0, 0, 0));
    int64_t numOfPixelPerTile = thisTile.total();

    for (int64_t it = 0; it < numOfPixelPerTile; ++it) {
        uint32_t p = osrRegion[it];

        uint8_t a = (p >> 24) & 0xFF;
        uint8_t r = (p >> 16) & 0xFF;
        uint8_t g = (p >> 8) & 0xFF;
        uint8_t b = p & 0xFF;

        switch (a) {
            case 0:
                r = 0;
                b = 0;
                g = 0;
                break;
            case 255:
                // no action needed
                break;
            default:
                r = (r * 255 + a / 2) / a;
                g = (g * 255 + a / 2) / a;
                b = (b * 255 + a / 2) / a;
                break;
        }

        // write back
        thisTile.at<cv::Vec3b>(it)[0] = b;
        thisTile.at<cv::Vec3b>(it)[1] = g;
        thisTile.at<cv::Vec3b>(it)[2] = r;
    }

    delete[] osrRegion;

    return;
}

void osrFilenameToCVMat(std::string filename, cv::Mat& matOut) {
    openslide_t* osr;
    int64_t w = -1;
    int64_t h = -1;

    // Opens svs input file
    osr = openslide_open(filename.c_str());

    // Gets info of largest image
    openslide_get_level0_dimensions(osr, &w, &h);

    // Gets the cv image from the osr format
    cv::Rect_<int64_t> roi(0, 0, w, h);
    osrRegionToCVMat(osr, roi, 0, matOut);
}
