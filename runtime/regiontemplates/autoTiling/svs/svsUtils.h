#ifndef SVS_UTILS_H_
#define SVS_UTILS_H_

#include <string>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "Util.h"

// Verifies whether the extension is .svs 
bool isSVS(std::string path);

// Extracts a roi from osr described by r, outputting it into thisTile
void osrRegionToCVMat(openslide_t* osr, cv::Rect_<int64_t> r, 
    int level, cv::Mat& thisTile);

void osrFilenameToCVMat(std::string filename, cv::Mat& matOut, int level=0);

#endif