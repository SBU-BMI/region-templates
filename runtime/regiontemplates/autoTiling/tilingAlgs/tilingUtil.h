#ifndef TILING_UTIL_H_
#define TILING_UTIL_H_

#include <cmath>
#include <list>
#include <string>
#include <iostream>

#include <opencv/cv.hpp>

// Pre-tiling algs types
enum PreTilerAlg_t {
    NO_PRE_TILER,
    DENSE_BG_SEPARATOR,
};

// Tiling algs types
enum TilerAlg_t {
    NO_TILER,
    LIST_ALG_HALF,
    LIST_ALG_EXPECT,
    KD_TREE_ALG_AREA,
    KD_TREE_ALG_COST,
    FIXED_GRID_TILING, // Here only for testing: parameter on PipelineManager
    TRIE_QUAD_TREE_ALG,
    POINT_QUAD_TREE_ALG,
};

// This representation makes the algorithms easier to implement and understand
// Used for all tiling algorithms
typedef struct rect_t {
    rect_t() : isBg(false) {}
    rect_t(int64_t ixi, int64_t iyi, int64_t ixo, int64_t iyo) : 
        xi(ixi), yi(iyi), xo(ixo), yo(iyo), isBg(false) {}
    rect_t(int64_t ixi, int64_t iyi, int64_t ixo, int64_t iyo, bool bg) :
        xi(ixi), yi(iyi), xo(ixo), yo(iyo), isBg(bg) {}
    
    bool operator==(const rect_t r) const{
        if(r.xi == xi && r.xo == xo 
            && r.yi == yi && r.yo == yo 
            && r.isBg == isBg) return true;
        else return false;
    }

    bool operator!=(const rect_t r) const{
        return !(*this == r);
    }

    int64_t xi, yi;
    int64_t xo, yo;
    bool isBg;
} rect_t;

/*****************************************************************************/
/**                            I/O and Profiling                            **/
/*****************************************************************************/

// prints the std dev of a tile inside an image. Used for profiling
void stddev(std::list<rect_t> rs, const cv::Mat& img, std::string name);

// Prints a rect_t without the carriage return
void printRect(rect_t r);

// // Creates a png file of the tile
// void printRegions(cv::Mat final, std::list<rect_t> output, int i) {    
//     for (std::list<rect_t>::iterator r=output.begin(); r!=output.end(); r++) {
//         // draw areas for verification
//         if (r->isBg) {
//             // cv::rectangle(final, cv::Point(r->xi,r->yi), 
//             //     cv::Point(r->xo,r->yo),(0,0,0),3);
//             cv::rectangle(final, cv::Point(r->xi,r->yi), 
//                 cv::Point(r->xo,r->yo),(0,0,0),1);
//             printRect(*r);
//             std::cout << std::endl;
//         }
//     }

//     cv::imwrite("./maskf" + std::to_string(i) + ".png", final);
// }


// inline bool isInsideNI(int64_t x, int64_t y, rect_t r2) {
//     return r2.xi < x && r2.yi < y && r2.xo > x && r2.yo > y;
// }

// inline bool isInsideNI(rect_t r1, rect_t r2) {
//     return isInsideNI(r1.xi, r1.yi, r2) && isInsideNI(r1.xo, r1.yo, r2);
// }

// inline bool overlaps(rect_t r1, rect_t r2) {
//     return isInsideNI(r1.xi, r1.yi, r2) || isInsideNI(r1.xi, r1.yo, r2) ||
//         isInsideNI(r1.xo, r1.yi, r2) || isInsideNI(r1.xo, r1.yo, r2) ||
//         isInsideNI(r2.xi, r2.yi, r1) || isInsideNI(r2.xi, r2.yo, r1) ||
//         isInsideNI(r2.xo, r2.yi, r1) || isInsideNI(r2.xo, r2.yo, r1);
// }

/*****************************************************************************/
/**                            Cost Calculations                            **/
/*****************************************************************************/

inline int64_t cost(const cv::Mat& img) {
    return cv::sum(img)[0];
}

inline int64_t cost(const cv::Mat& img, const rect_t& r) {
    return cv::sum(img(cv::Range(r.yi, r.yo), cv::Range(r.xi, r.xo)))[0];
}

inline int64_t cost(const cv::Mat& img, int64_t yi, int64_t yo, 
        int64_t xi, int64_t xo) {
    return cv::sum(img(cv::Range(yi, yo), cv::Range(xi, xo)))[0];
}

inline bool between(int64_t x, int64_t init, int64_t end) {
    return x >= init && x <= end;
}

inline int64_t area (rect_t r) {
    return (r.xo-r.xi) * (r.yo-r.yi);
}

/*****************************************************************************/
/**                           Log Split Algorithm                           **/
/*****************************************************************************/

// Performs a binary sweep search across the input tile image, searching for 
// a split on which the goal cost is found within an acc margin of error.
// At least one of the new tiles must have the goal cost. Both a vertical
// and a horizontal sweep are performed, being returned the pair of regions
// with the smallest difference between areas.
// Orient: 0 = both, -1 = horizontal only, +1 = vetical only
void splitTileLog(const rect_t& r, const cv::Mat& img, int expCost, 
    rect_t& newt1, rect_t& newt2, float acc=0.2, int orient = 0);

#endif // UTIL_H_
