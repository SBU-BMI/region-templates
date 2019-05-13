#include "DenseSvsDataRegion2D.h"

DenseSvsDataRegion2D::DenseSvsDataRegion2D(cv::Rect_<int64_t> roi) 
    : DenseDataRegion2D() {

    this->hasData = false;
    this->hasMetadata = false;
    this->roi = roi;
    this->svsFile = NULL;
    this->maxLevel = -1;

    std::cout << "[DenseSvsDataRegion2D] construct-------------------------" << std::endl;
}

DenseSvsDataRegion2D::~DenseSvsDataRegion2D() {
    // if (this->svsFile != NULL) {
    //     openslide_close(this->svsFile);
    // }
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

void DenseSvsDataRegion2D::getMatMetadata(ExecutionEngine* env) {
std::cout << "getM of tile " << this->roi.x << "," << this->roi.y << std::endl;
    if (this->svsFile != NULL) {
        // this->svsFile = openslide_open(this->getInputFileName().c_str());
        this->svsFile = env->getSvsPointer(this->getInputFileName());
    }

    // REPLICATED CODE
    // Gets the largest level
    int32_t levels = openslide_get_level_count(this->svsFile);
    int64_t w, h;
    int64_t maxSize = -1;
    for (int32_t l=0; l<levels; l++) {
        openslide_get_level_dimensions(this->svsFile, l, &w, &h);
        if (h*w > maxSize) {
            maxSize = h*w;
            this->maxLevel = l;
        }
    }

    openslide_get_level_dimensions(this->svsFile, 
        this->maxLevel, &this->w, &this->h);

    this->hasMetadata = true;
}

void DenseSvsDataRegion2D::getMatData(ExecutionEngine* env) {
std::cout << "getD of tile " << this->roi.x << "," << this->roi.y << std::endl;
    if (!this->hasMetadata) {
        getMatMetadata(env);
    }

    cv::Mat mat;
    osrRegionToCVMat2(this->svsFile, this->roi, this->maxLevel, mat);
    this->setData(mat);

    this->hasData = true;
}

cv::Mat DenseSvsDataRegion2D::getData(ExecutionEngine* env) {
    std::cout << "[DenseSvsDataRegion2D] getData" << std::endl;
    if (!this->hasData) {
        std::cout << "[DenseSvsDataRegion2D] getData new" << std::endl;
        getMatData(env);
    }

    return DenseDataRegion2D::getData();
}

// int DenseSvsDataRegion2D::serialize(char* buff) {
//     int serialized_bytes = DataRegion::serialize(buff);

//     bool hasData = this->hasData;
//     memcpy(buff+serialized_bytes, &hasData, sizeof(bool));
//     serialized_bytes += sizeof(bool);

//     bool hasMetadata = this->hasMetadata;
//     memcpy(buff+serialized_bytes, &hasMetadata, sizeof(bool));
//     serialized_bytes += sizeof(bool);

//     int64_t x = this->roi.x;
//     memcpy(buff+serialized_bytes, &x, sizeof(int64_t));
//     serialized_bytes += sizeof(int64_t);

//     int64_t y = this->roi.y;
//     memcpy(buff+serialized_bytes, &y, sizeof(int64_t));
//     serialized_bytes += sizeof(int64_t);

//     int64_t w = this->roi.width;
//     memcpy(buff+serialized_bytes, &w, sizeof(int64_t));
//     serialized_bytes += sizeof(int64_t);

//     int64_t h = this->roi.height;
//     memcpy(buff+serialized_bytes, &h, sizeof(int64_t));
//     serialized_bytes += sizeof(int64_t);

//     return serialized_bytes;
// }

// int DenseSvsDataRegion2D::deserialize(char* buff) {
//     int deserialized_bytes = DataRegion::deserialize(buff);

//     this->hasData = ((bool*)(buff+deserialized_bytes))[0];;
//     deserialized_bytes += sizeof(bool);

//     this->hasMetadata = ((bool*)(buff+deserialized_bytes))[0];;
//     deserialized_bytes += sizeof(bool);

//     int64_t x = ((int64_t*)(buff+deserialized_bytes))[0];;
//     deserialized_bytes += sizeof(int64_t);
//     int64_t y = ((int64_t*)(buff+deserialized_bytes))[0];;
//     deserialized_bytes += sizeof(int64_t);
//     int64_t w = ((int64_t*)(buff+deserialized_bytes))[0];;
//     deserialized_bytes += sizeof(int64_t);
//     int64_t h = ((int64_t*)(buff+deserialized_bytes))[0];;
//     deserialized_bytes += sizeof(int64_t);

//     this->roi = cv::Rect_<int64_t>(x, y, w, h);

//     return deserialized_bytes;
// }

// int DenseSvsDataRegion2D::serializationSize() {
//     int size_bytes = DataRegion::serializationSize();

//     // hasData
//     size_bytes += sizeof(bool);

//     // hasMetadata
//     size_bytes += sizeof(bool);

//     // roi
//     size_bytes += 4 * sizeof(int64_t);

//     return size_bytes;
// }
