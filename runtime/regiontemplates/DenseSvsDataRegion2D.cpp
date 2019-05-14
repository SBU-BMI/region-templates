#include "DenseSvsDataRegion2D.h"

DenseSvsDataRegion2D::DenseSvsDataRegion2D(cv::Rect_<int64_t> roi) 
    : DenseDataRegion2D() {

    this->roi = roi;
}

// REPLICATED CODE
void osrRegionToCVMat2(openslide_t* osr, cv::Rect_<int64_t> r, 
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

void DenseSvsDataRegion2D::printRoi() {
    std::cout << "[DenseSvsDataRegion2D] printRoi" << std::endl;
    std::cout << "tile: " << this->roi.x << "," << this->roi.y 
        << ":" << this->roi.width << "," << this->roi.height << std::endl;
}

cv::Mat DenseSvsDataRegion2D::getData(ExecutionEngine* env) {
    // Gets the svs file pointer from local env cache
    openslide_t* svsFile = env->getSvsPointer(this->getInputFileName());

    // REPLICATED CODE
    // Gets the largest level
    int32_t levels = openslide_get_level_count(svsFile);
    int64_t w, h;
    int64_t maxSize = -1;
    int32_t maxLevel;
    for (int32_t l=0; l<levels; l++) {
        openslide_get_level_dimensions(svsFile, l, &w, &h);
        if (h*w > maxSize) {
            maxSize = h*w;
            maxLevel = l;
        }
    }

    cv::Mat mat;
    osrRegionToCVMat2(svsFile, this->roi, maxLevel, mat);
    this->setData(mat);

    return DenseDataRegion2D::getData();
}
