#ifndef COST_FUNCTION_H_
#define COST_FUNCTION_H_

#include <iostream>
#include <opencv/cv.hpp>

// template <typename T>
class CostFunction {
public:
    CostFunction() {};

    // virtual T cost(cv::Mat img) = 0;
    virtual int64_t cost(cv::Mat img) {std::cout << "bad cost..." << std::endl; exit(-1);};
    int64_t cost(cv::Mat img, cv::Rect_<int64_t> tile);
};

#endif
