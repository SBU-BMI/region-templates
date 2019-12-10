#include "CostFunction.h"

int64_t CostFunction::cost(cv::Mat img, cv::Rect_<int64_t> tile) {
    return this->cost(img(cv::Range(tile.y, tile.y+tile.height), 
                       cv::Range(tile.x, tile.x+tile.width)));
}
