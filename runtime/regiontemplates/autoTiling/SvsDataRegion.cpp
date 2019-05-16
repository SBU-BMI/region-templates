#include "SvsDataRegion.h"

SvsDataRegion::SvsDataRegion() {
    this->setType(DataRegionType::DENSE_SVS_REGION_2D);
    this->setSvs();
}

cv::Mat SvsDataRegion::getData(ExecutionEngine* env) {
    // Gets the svs file pointer from local env cache
    openslide_t* svsFile = env->getSvsPointer(this->getInputFileName());

    // Extracts the roi of the svs file into a mat
    cv::Mat mat;
    int32_t maxLevel = 0; // svs standard: maxlevel = 0
    osrRegionToCVMat(svsFile, this->roi, maxLevel, mat);

    return mat;
}
