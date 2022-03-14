#ifndef TILING_UTIL_H_
#define TILING_UTIL_H_

#include <algorithm>
#include <cmath>
#include <iostream>
#include <list>
#include <string>

#include <opencv/cv.hpp>

#include "CostFunction.h"

// Pre-tiling algs types
enum PreTilerAlg_t {
    NO_PRE_TILER,
    DENSE_BG_SEPARATOR,
};

// Tiling algs types
enum TilerAlg_t {
    FIXED_GRID_TILING,
    LIST_ALG_HALF,
    LIST_ALG_EXPECT,
    KD_TREE_ALG_AREA,
    KD_TREE_ALG_COST,
    HBAL_TRIE_QUAD_TREE_ALG,
    CBAL_TRIE_QUAD_TREE_ALG,
    CBAL_POINT_QUAD_TREE_ALG,
    TILING1D,
};

/*****************************************************************************/
/**                            rect_t Structure                             **/
/*****************************************************************************/

// This representation makes the algorithms easier to implement and understand
// Used for all tiling algorithms
typedef struct rect_t {
    rect_t() : isBg(false) {}
    rect_t(int64_t ixi, int64_t iyi, int64_t ixo, int64_t iyo)
        : xi(ixi), yi(iyi), xo(ixo), yo(iyo), isBg(false) {}
    rect_t(int64_t ixi, int64_t iyi, int64_t ixo, int64_t iyo, bool bg)
        : xi(ixi), yi(iyi), xo(ixo), yo(iyo), isBg(bg) {}

    bool operator==(const rect_t r) const {
        if (r.xi == xi && r.xo == xo && r.yi == yi && r.yo == yo &&
            r.isBg == isBg)
            return true;
        else
            return false;
    }

    bool operator!=(const rect_t r) const { return !(*this == r); }

    int64_t xi, yi;
    int64_t xo, yo;
    bool    isBg;
} rect_t;

std::list<rect_t> toMyRectT(std::list<cv::Rect_<uint64_t>> from);

/*****************************************************************************/
/**                            Cost Calculations                            **/
/*****************************************************************************/

// inline int64_t cost(const cv::Mat& img) {
//     return cv::sum(img)[0];
// }

// inline int64_t cost(const cv::Mat& img, const rect_t& r) {
//     return cv::sum(img(cv::Range(r.yi, r.yo), cv::Range(r.xi, r.xo)))[0];
// }

// template <typename T>
// inline int64_t cost(const cv::Mat& img, const cv::Rect_<T>& r) {
//     return cv::sum(img(cv::Range(r.y, r.y+r.height),
//                        cv::Range(r.x, r.x+r.width)))[0];
// }

// inline int64_t cost(const cv::Mat& img, int64_t yi, int64_t yo,
//         int64_t xi, int64_t xo) {
//     return cv::sum(img(cv::Range(yi, yo), cv::Range(xi, xo)))[0];
// }

inline bool between(int64_t x, int64_t init, int64_t end) {
    return x >= init && x <= end;
}

inline int64_t area(rect_t r) { return (r.xo - r.xi) * (r.yo - r.yi); }

// // Functor for sorting a container of rect_t using the added cost of said
// // region on the img parameter.
// // NOTE: when instantiating container, use double parenthesis to avoid
// // compiler confusion of the declared object with a function:
// // Cont<t1, rect_tCostFunct> obj((rect_tCostFunct(img)))

struct rect_tCostFunct {
    const cv::Mat      &img;
    const CostFunction *cfunc;
    rect_tCostFunct(const cv::Mat &img, CostFunction *cfunc)
        : img(img), cfunc(cfunc) {}
    bool operator()(const rect_t &a, const rect_t &b) {
        return this->cfunc->cost(this->img, a.yi, a.yo, a.xi, a.xo) >
               this->cfunc->cost(this->img, b.yi, b.yo, b.xi, b.xo);
    }
};

/*****************************************************************************/
/**                            I/O and Profiling                            **/
/*****************************************************************************/

// prints the std dev of a tile inside an image. Used for profiling
void stddev(std::list<rect_t> rs, const cv::Mat &img, std::string name);

// Prints a rect_t without the carriage return
void printRect(rect_t r);

/*****************************************************************************/
/**                           Log Split Algorithm                           **/
/*****************************************************************************/

// Performs a binary sweep search across the input tile image, searching for
// a split on which the goal cost is found within an acc margin of error.
// At least one of the new tiles must have the goal cost. Both a vertical
// and a horizontal sweep are performed, being returned the pair of regions
// with the smallest difference between areas.
// Orient: 0 = both, -1 = horizontal only, +1 = vetical only
void splitTileLog(const rect_t &r, const cv::Mat &img, CostFunction *cfunc,
                  double expCost, rect_t &newt1, rect_t &newt2,
                  float acc = 0.02, int orient = 0);

#endif // UTIL_H_
