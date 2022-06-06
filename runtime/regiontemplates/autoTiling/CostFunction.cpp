#include "CostFunction.h"

double CostFunction::cost(const cv::Mat &img, cv::Rect_<int64_t> tile) const {
    // std::cout << "[CostFunction::cost][TILING] " << tile.x << ":"
    //           << (tile.x + tile.width) << ", " << tile.y << ":"
    //           << (tile.y + tile.height) << "\n";
    return this->cost(img(cv::Range(tile.y, tile.y + tile.height),
                          cv::Range(tile.x, tile.x + tile.width)));
}

double CostFunction::cost(const cv::Mat &img, int64_t yi, int64_t yo,
                          int64_t xi, int64_t xo) const {
    // std::cout << "[CostFunction::cost][TILING] " << xi << ":" << xo << ", "
    //           << yi << ":" << yo << "\n";
    return this->cost(img(cv::Range(yi, yo), cv::Range(xi, xo)));
}

cv::Mat CostFunction::costImg(const cv::Mat     &img,
                              cv::Rect_<int64_t> tile) const {
    return this->costImg(img(cv::Range(tile.y, tile.y + tile.height),
                             cv::Range(tile.x, tile.x + tile.width)));
}

cv::Mat CostFunction::costImg(const cv::Mat &img, int64_t yi, int64_t yo,
                              int64_t xi, int64_t xo) const {

    return this->costImg(img(cv::Range(yi, yo), cv::Range(xi, xo)));
}
