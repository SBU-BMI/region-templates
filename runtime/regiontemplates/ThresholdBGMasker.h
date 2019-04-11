#ifndef THRESHOLD_BG_MASKER_H_
#define THRESHOLD_BG_MASKER_H_

#include <string>

#include <opencv/cv.hpp>

#include "BGMasker.h"

#define CV_MAX_PIX_VAL 255
#define CV_THR_BIN 0
#define CV_THR_BIN_INV 1

class ThresholdBGMasker : public BGMasker {
private:
    int bgThr;
    int erode;
public:
    ThresholdBGMasker(int bgThr, int erode);

    cv::Mat bgMask(cv::Mat img);
};

#endif
