#include "SvsDataRegion.h"

SvsDataRegion::SvsDataRegion() {
    this->setType(DataRegionType::DENSE_SVS_REGION_2D);
    this->setSvs();
}

void SvsDataRegion::setRoi(cv::Rect_<int64_t> roi) {
    this->roi = roi;
}

int SvsDataRegion::serialize(char* buff) {
    int serialized_bytes = DataRegion::serialize(buff);

    // roi of svs files
    int64_t x = this->roi.x;
    memcpy(buff+serialized_bytes, &x, sizeof(int64_t));
    serialized_bytes += sizeof(int64_t);
    int64_t y = this->roi.y;
    memcpy(buff+serialized_bytes, &y, sizeof(int64_t));
    serialized_bytes += sizeof(int64_t);
    int64_t w = this->roi.width;
    memcpy(buff+serialized_bytes, &w, sizeof(int64_t));
    serialized_bytes += sizeof(int64_t);
    int64_t h = this->roi.height;
    memcpy(buff+serialized_bytes, &h, sizeof(int64_t));
    serialized_bytes += sizeof(int64_t);

    return serialized_bytes;
}

int SvsDataRegion::deserialize(char* buff) {
    int deserialized_bytes = DataRegion::deserialize(buff);

    // roi of svs files
    int64_t x = ((int64_t*)(buff+deserialized_bytes))[0];;
    deserialized_bytes += sizeof(int64_t);
    int64_t y = ((int64_t*)(buff+deserialized_bytes))[0];;
    deserialized_bytes += sizeof(int64_t);
    int64_t w = ((int64_t*)(buff+deserialized_bytes))[0];;
    deserialized_bytes += sizeof(int64_t);
    int64_t h = ((int64_t*)(buff+deserialized_bytes))[0];;
    deserialized_bytes += sizeof(int64_t);
    this->roi = cv::Rect_<int64_t>(x, y, w, h);

    return deserialized_bytes;
}

int SvsDataRegion::serializationSize() {
    int size_bytes = DataRegion::serializationSize();

    // roi of svs files
    size_bytes += 4 * sizeof(int64_t);

    return size_bytes;
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

void SvsDataRegion::printRoi() {
    std::cout << "[SvsDataRegion] printRoi" << std::endl;
    std::cout << "tile: " << this->roi.x << "," << this->roi.y 
        << ":" << this->roi.width << "," << this->roi.height << std::endl;
}

cv::Mat SvsDataRegion::getData(ExecutionEngine* env) {
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

    // Extracts the roi of the svs file into a mat
    cv::Mat mat;
    osrRegionToCVMat2(svsFile, this->roi, maxLevel, mat);

    return mat;
}