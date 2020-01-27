#include "CostFunction.h"

int64_t CostFunction::cost(cv::Mat img, cv::Rect_<int64_t> tile) {
    return this->cost(img(cv::Range(tile.y, tile.y+tile.height), 
                       cv::Range(tile.x, tile.x+tile.width)));
}

int64_t CostFunction::cost(cv::Mat img, int64_t yi, int64_t yo, 
    int64_t xi, int64_t xo) {

    return this->cost(img(cv::Range(yi, yo), cv::Range(xi, xo)));
}

cv::Mat CostFunction::costImg(cv::Mat img, cv::Rect_<int64_t> tile) {
    return this->costImg(img(cv::Range(tile.y, tile.y+tile.height), 
                       cv::Range(tile.x, tile.x+tile.width)));
}

cv::Mat CostFunction::costImg(cv::Mat img, int64_t yi, int64_t yo, 
    int64_t xi, int64_t xo) {

    return this->costImg(img(cv::Range(yi, yo), cv::Range(xi, xo)));
}

