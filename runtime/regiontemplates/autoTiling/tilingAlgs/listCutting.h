#ifndef LIST_CUTTING_H_
#define LIST_CUTTING_H_

#include <list>
#include <set>
#include <iostream>

#include <opencv/cv.hpp>

#include "tilingUtil.h"

// Functor for sorting a container of rect_t using the added cost of said
// region on the img parameter.
// NOTE: when instantiating container, use double parenthesis to avoid 
// compiler confusion of the declared object with a function:
// Cont<t1, rect_tCostFunct> obj((rect_tCostFunct(img)))
struct rect_tCostFunct{
    const cv::Mat& img;
    rect_tCostFunct(const cv::Mat& img) : img(img) {}
    bool operator()(const rect_t& a, const rect_t& b) {
        return cost(img, a) > cost(img, b);
    }
};

void listCutting(const cv::Mat& img, std::list<rect_t>& dense, 
    int nTiles, TilerAlg_t type);

#endif // LIST_CUTTING_H_
