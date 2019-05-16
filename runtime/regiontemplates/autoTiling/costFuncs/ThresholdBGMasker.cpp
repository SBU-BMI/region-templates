#include "ThresholdBGMasker.h"

ThresholdBGMasker::ThresholdBGMasker(int bgThr, int dilate, int erode) {
    this->bgThr = bgThr;
    this->dilate = dilate;
    this->erode = erode;
}

cv::Mat ThresholdBGMasker::bgMask(cv::Mat img) {
    cv::Mat mask;

    // If the input img is colored, convert it to grayscale
    if (img.channels() == 3)
        cv::cvtColor(img, img, cv::COLOR_RGB2GRAY);

    // create a binary threshold mask of the input img
    cv::threshold(img, mask, bgThr, CV_MAX_PIX_VAL, CV_THR_BIN_INV); 
    
    // erode the threshold mask
    cv::Mat dElement = cv::getStructuringElement(cv::MORPH_RECT,
        cv::Size(2*dilate, 2*dilate));
    cv::Mat eElement = cv::getStructuringElement(cv::MORPH_RECT,
        cv::Size(2*dilate+2, 2*dilate+2));
    cv::dilate(mask, mask, dElement);
    cv::erode(mask, mask, eElement);

    return mask;
};

