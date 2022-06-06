#include "MultiObjCostFunction.h"

#include <cstdlib>
#include <iostream>

MultiObjCostFunction::MultiObjCostFunction(int bgThr, int dilate, int erode,
                                           float execBias, float readBias)
    : execBias(execBias), readBias(readBias) {
    this->bgm = new ThresholdBGMasker(bgThr, dilate, erode);
}

MultiObjCostFunction::MultiObjCostFunction(BGMasker *bgm, float execBias,
                                           float readBias)
    : bgm(bgm), execBias(execBias), readBias(readBias) {}

// Cost is approximated as a parameterized linear function of execution cost
// and image loading time, which is proportional to its size.
double MultiObjCostFunction::cost(const cv::Mat &img) const {
    // dense mask cost
    cv::Mat bgImg    = this->bgm->bgMask(img);
    double  execCost = cv::sum(cv::sum(bgImg))[0];

    double readCost = img.cols * img.rows;

    int min = 70; // error margin
    // returns rand between 1-min/100 and 1
    // double randpct =
    //     (double)(1) + ((double)(rand() % min) / 100) - ((double)(min) / 200);
    double randpct = 1;
    // std::cout << "===============error: " << randpct << std::endl;

    return randpct * (execCost * this->execBias + readCost * this->readBias);
}

cv::Mat MultiObjCostFunction::costImg(const cv::Mat &img) const {
    return this->bgm->bgMask(img);
}