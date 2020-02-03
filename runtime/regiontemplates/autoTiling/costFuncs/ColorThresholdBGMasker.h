#ifndef COLOR_THRESHOLD_BG_MASKER_H_
#define COLOR_THRESHOLD_BG_MASKER_H_

#include <string>

#include <opencv/cv.hpp>

#include "BGMasker.h"

#define CV_MAX_PIX_VAL 255
#define CV_THR_BIN 0
#define CV_THR_BIN_INV 1


class ColorThresholdBGMasker : public BGMasker {
private:
    int blue;
    int green;
    int red;
    int dilate;
    int erode;
    
public:
    ColorThresholdBGMasker(int dilate, int erode, 
    	uint8_t blue=200, uint8_t green=200, uint8_t red=200);

    cv::Mat bgMask(cv::Mat img);
};

#endif
