#include "ThresholdBGCostFunction.h"

// template <typename T>
// ThresholdBGCostFunction<T>::ThresholdBGCostFunction(int bgThr, int dilate, int erode) {
ThresholdBGCostFunction::ThresholdBGCostFunction(int bgThr, int dilate, int erode) {
    this->bgm = new ThresholdBGMasker(bgThr, dilate, erode);
}

// template <typename T>
// ThresholdBGCostFunction<T>::ThresholdBGCostFunction(ThresholdBGMasker bgm) {
ThresholdBGCostFunction::ThresholdBGCostFunction(ThresholdBGMasker* bgm) {
    this->bgm = bgm;
}

// template <typename T>
// T ThresholdBGCostFunction<T>::cost(cv::Mat img) {
int64_t ThresholdBGCostFunction::cost(cv::Mat img) const {
    cv::Mat bgImg = this->bgm->bgMask(img);
    return cv::sum(cv::sum(bgImg))[0];
}

cv::Mat ThresholdBGCostFunction::costImg(cv::Mat img) const {
    return this->bgm->bgMask(img);
}