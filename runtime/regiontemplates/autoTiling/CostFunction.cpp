#include "CostFunction.h"

double CostFunction::cost(cv::Mat img, cv::Rect_<int64_t> tile) const {
    return this->cost(img(cv::Range(tile.y, tile.y+tile.height), 
                       cv::Range(tile.x, tile.x+tile.width)));
}

double CostFunction::cost(cv::Mat img, int64_t yi, int64_t yo, 
    int64_t xi, int64_t xo) const {

    return this->cost(img(cv::Range(yi, yo), cv::Range(xi, xo)));
}

cv::Mat CostFunction::costImg(cv::Mat img, cv::Rect_<int64_t> tile) const {
    return this->costImg(img(cv::Range(tile.y, tile.y+tile.height), 
                       cv::Range(tile.x, tile.x+tile.width)));
}

cv::Mat CostFunction::costImg(cv::Mat img, int64_t yi, int64_t yo, 
    int64_t xi, int64_t xo) const {

    return this->costImg(img(cv::Range(yi, yo), cv::Range(xi, xo)));
}

