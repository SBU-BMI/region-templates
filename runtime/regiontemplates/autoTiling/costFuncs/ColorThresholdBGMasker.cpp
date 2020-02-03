#include "ColorThresholdBGMasker.h"

ColorThresholdBGMasker::ColorThresholdBGMasker(int dilate, int erode, 
        uint8_t blue, uint8_t green, uint8_t red) {
    this->blue = blue;
    this->green = green;
    this->red = red;
    this->dilate = dilate;
    this->erode = erode;
}

cv::Mat ColorThresholdBGMasker::bgMask(cv::Mat img) {
    cv::Mat mask;

    // Input img needs to be colored
    assert(img.channels() == 3);

    std::vector<cv::Mat> bgr;
    cv::split(img, bgr);
    mask = (bgr[0] > blue) & (bgr[1] > green) & (bgr[2] > red);

    // ensures that binary image is 0/255
    cv::threshold(mask, mask, 1, CV_MAX_PIX_VAL, CV_THR_BIN_INV); 
    
    // erode the threshold mask
    cv::Mat dElement = cv::getStructuringElement(cv::MORPH_RECT,
        cv::Size(2*this->dilate, 2*this->dilate));
    cv::Mat eElement = cv::getStructuringElement(cv::MORPH_RECT,
        cv::Size(2*this->erode+2, 2*this->erode+2));
    cv::erode(mask, mask, eElement);
    cv::dilate(mask, mask, dElement);

    return mask;
};

