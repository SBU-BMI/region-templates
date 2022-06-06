#include "PrePartThresBGCostFunction.h"

#include <boost/stacktrace.hpp>
#include <omp.h>

#include "openslide.h"

#include "ThresholdBGCostFunction.h"
#include "svs/svsUtils.h"

// cv::Mat calculateCostArray(CostFunction *cFunc, ThresholdBGMasker *bgm,
//                            std::string origImgPath, int xRes, int yRes) {

//     long t1 = Util::ClockGetTime();
//     // Opens svs input file
//     int64_t      w = -1;
//     int64_t      h = -1;
//     openslide_t *osr;
//     int32_t      osrMinLevel = -1;
//     osr                      = openslide_open(origImgPath.c_str());

//     // Opens smallest image as a cv mat
//     osrMinLevel = openslide_get_level_count(osr) - 1; // last level
//     openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
//     cv::Rect_<int64_t> roi(0, 0, w, h);
//     cv::Mat            origImg;
//     osrRegionToCVMat(osr, roi, osrMinLevel, origImg);

//     // Close .svs file
//     openslide_close(osr);
//     long t2 = Util::ClockGetTime();

//     // Get Mask
//     // std::cout << "============1===========\n";
//     // origImg = bgm->bgMask(origImg);

//     int xCount = std::ceil(origImg.cols / xRes);
//     int yCount = std::ceil(origImg.rows / yRes);
//     // std::cout << "=== w: " << w << "\n";
//     // std::cout << "=== h: " << h << "\n";

//     // Create output costs array
//     // std::cout << "===========2============\n";
//     cv::Mat costArray(yCount, xCount, CV_64FC1);

//     // std::cout << "==========3=============\n";
//     long t3 = Util::ClockGetTime();
//     std::cout << "[TILING_CONSTRUCT] xCount " << xCount << "\n";
//     std::cout << "[TILING_CONSTRUCT] yCount " << yCount << "\n";

//     // Assign costs for each tile
//     // omp_set_num_threads(16);
// #pragma omp parallel for shared(costArray, origImg, cFunc)
//     for (int i = 0; i < xCount; i++) {
//         for (int j = 0; j < yCount; j++) {
//             long width  = std::min((j + 1) * yRes, origImg.rows);
//             long height = std::min((i + 1) * xRes, origImg.cols);
//             // std::cout << "h: " << height << "\n";
//             // std::cout << "w: " << width << "\n";
//             // std::cout << "====" << i << "=" << j <<
//             "==================\n"; costArray.at<double>(cv::Point(i, j)) =
//                 cFunc->cost(origImg, j * yRes, width, i * xRes, height);
//         }
//     }
//     long t4 = Util::ClockGetTime();

//     std::cout << "[TILING_CONSTRUCT] svs_open " << (t2 - t1) << "\n";
//     std::cout << "[TILING_CONSTRUCT] cvMat_alloc " << (t3 - t2) << "\n";
//     std::cout << "[TILING_CONSTRUCT] calc " << (t4 - t3) << "\n";

//     return costArray;
// }

PrePartThresBGCostFunction::PrePartThresBGCostFunction(int bgThr, int dilate,
                                                       int         erode,
                                                       std::string origImgPath,
                                                       int xRes, int yRes) {
    this->bgm   = new ThresholdBGMasker(bgThr, dilate, erode);
    this->cFunc = new ThresholdBGCostFunction(bgThr, dilate, erode);

    // Opens svs input file
    int64_t      w = -1;
    int64_t      h = -1;
    openslide_t *osr;
    int32_t      osrMinLevel = -1;
    osr                      = openslide_open(origImgPath.c_str());

    // Opens smallest image as a cv mat
    osrMinLevel = openslide_get_level_count(osr) - 1; // last level
    openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
    cv::Rect_<int64_t> roi(0, 0, w, h);
    osrRegionToCVMat(osr, roi, osrMinLevel, this->origImg);

    // Close .svs file
    openslide_close(osr);

    if (xRes == -1)
        xRes = std::ceil(this->origImg.cols / oneByPct); // 1% of coord

    if (yRes == -1)
        yRes = std::ceil(this->origImg.rows / oneByPct); // 1% of coord

    this->xResolution = xRes;
    this->yResolution = yRes;

    std::cout << "[PrePartThresBGCostFunction] xres: " << xRes << "\n";
    std::cout << "[PrePartThresBGCostFunction] yres: " << yRes << "\n";

    int xCount      = std::ceil(this->origImg.cols / xRes);
    int yCount      = std::ceil(this->origImg.rows / yRes);
    this->costArray = cv::Mat(xCount, yCount, CV_64FC1, cv::Scalar(-1));
    // this->costArray =
    //     calculateCostArray(costFunc, bgm, origImgPath, xRes, yRes);
}

PrePartThresBGCostFunction::PrePartThresBGCostFunction(ThresholdBGMasker *bgm,
                                                       std::string origImgPath,
                                                       int xRes, int yRes) {
    this->cFunc = bgm->getCostFunction();

    // Opens svs input file
    int64_t      w = -1;
    int64_t      h = -1;
    openslide_t *osr;
    int32_t      osrMinLevel = -1;
    osr                      = openslide_open(origImgPath.c_str());

    // Opens smallest image as a cv mat
    osrMinLevel = openslide_get_level_count(osr) - 1; // last level
    openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
    cv::Rect_<int64_t> roi(0, 0, w, h);
    osrRegionToCVMat(osr, roi, osrMinLevel, this->origImg);

    // Close .svs file
    openslide_close(osr);

    if (xRes == -1)
        xRes = std::ceil(this->origImg.cols / oneByPct); // 1% of coord

    if (yRes == -1)
        yRes = std::ceil(this->origImg.rows / oneByPct); // 1% of coord

    this->xResolution = xRes;
    this->yResolution = yRes;

    std::cout << "[PrePartThresBGCostFunction] xres: " << xRes << "\n";
    std::cout << "[PrePartThresBGCostFunction] yres: " << yRes << "\n";
    int xCount      = std::ceil(this->origImg.cols / xRes);
    int yCount      = std::ceil(this->origImg.rows / yRes);
    this->costArray = cv::Mat(xCount, yCount, CV_64FC1, cv::Scalar(-1));
    // this->costArray =
    //     calculateCostArray(costFunc, bgm, origImgPath, xRes, yRes);
    this->bgm = bgm;
}

double PrePartThresBGCostFunction::cost(const cv::Mat &img) const {
    // std::cout << "[PrePartThresBGCostFunction] tried to calculate the cost of
    // "
    //              "another image.\n";
    // std::cout << boost::stacktrace::stacktrace();
    // exit(-8);
    return cv::sum(cv::sum(this->costArray))[0];
}

cv::Mat PrePartThresBGCostFunction::costImg(const cv::Mat &img) const {
    // std::cout << "[PrePartThresBGCostFunction] tried to calculate the cost "
    //              "mat of another image.\n";
    // std::cout << boost::stacktrace::stacktrace();
    // exit(-8);

    auto    costFunc = bgm->getCostFunction();
    cv::Mat costMat  = costFunc->costImg(img);
    delete costFunc;

    return costMat;
}

double PrePartThresBGCostFunction::cost(const cv::Mat     &_img,
                                        cv::Rect_<int64_t> tile) const {
    return this->cost(_img, tile.y, tile.y + tile.height, tile.x,
                      tile.x + tile.width);
}

double PrePartThresBGCostFunction::cost(const cv::Mat &_img, int64_t yi,
                                        int64_t yo, int64_t xi,
                                        int64_t xo) const {
    if ((xo - xi <= this->xResolution) || (yo - yi <= this->yResolution))
        return -1;

    int xiTile = std::round(xi / this->xResolution);
    int xoTile = std::round(xo / this->xResolution);
    int yiTile = std::round(yi / this->yResolution);
    int yoTile = std::round(yo / this->yResolution);

    // std::cout << "[PrePartThresBGCostFunction::cost][TILING] " << xi << ":"
    //           << xo << ", " << yi << ":" << yo << "\n";
    // std::cout << "[PrePartThresBGCostFunction::cost][TILING] " << xiTile <<
    // ":"
    //           << xoTile << ", " << yiTile << ":" << yoTile << "\n";

    // Go through all cost tiles
    double totalCost = 0;
    for (int i = xiTile; i < xoTile; i++) {
        for (int j = yiTile; j < yoTile; j++) {
            int yo = std::min((j + 1) * this->yResolution, this->origImg.rows);
            int xo = std::min((i + 1) * this->xResolution, this->origImg.cols);
            double cost = this->costArray.at<double>(cv::Point(i, j));
            // std::cout << "[loop][TILING] " << xiTile << ":" << xoTile << ", "
            //           << yiTile << ":" << yoTile << " = " << cost << "\n";
            if (cost == -1) {
                cost = this->cFunc->cost(this->origImg, j * this->yResolution,
                                         yo, i * this->xResolution, xo);
                this->costArray.at<double>(cv::Point(i, j)) = cost;
            }
            totalCost += cost;
            // exit(-1);
        }
    }

    // std::cout << yi << "\n";
    // std::cout << yo << "\n";
    // std::cout << xi << "\n";
    // std::cout << xo << "\n";

    // cv::Mat subMat =
    //     this->costArray(cv::Range(yiTile, yoTile), cv::Range(xiTile,
    //     xoTile));
    // double cost = cv::sum(cv::sum(subMat))[0];

    return totalCost;
}