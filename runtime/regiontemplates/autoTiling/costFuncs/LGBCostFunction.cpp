#include "LGBCostFunction.h"

#include <LightGBM/boosting.h>
#include <LightGBM/prediction_early_stop.h>

#include <iostream>

#define PI 3.1415

LGBCostFunction::LGBCostFunction(int bgThr, int dilate, int erode) {
    this->bgm = new ThresholdBGMasker(bgThr, dilate, erode);
}

LGBCostFunction::LGBCostFunction(ThresholdBGMasker *bgm) { this->bgm = bgm; }

double LGBCostFunction::cost(const cv::Mat &img) const {

    cv::Mat bgImg = this->bgm->bgMask(img);

    // === Prepare stats from largest region ==================================
    cv::Mat labels, stats, centroids;
    int     nLabels =
        cv::connectedComponentsWithStats(bgImg, labels, stats, centroids);

    cv::Mat                             hierarchy;
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(bgImg, contours, hierarchy, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);

    int  bigContId = -1;
    long maxArea   = 0;
    for (int i = 0; i < contours.size(); i++) {
        auto area = cv::contourArea(contours[i], false);
        if (area > maxArea) {
            maxArea   = area;
            bigContId = i;
        }
    }

    int bigBBId = -1;
    maxArea     = 0;
    for (int i = 0; i < nLabels; i++) {
        auto area = stats.at<int>(i, cv::CC_STAT_AREA);
        if (area > maxArea) {
            maxArea = area;
            bigBBId = i;
        }
    }

    // If no contour is found, return cost zero
    if (bigContId == -1 || bigBBId == -1)
        return 0.0;

    // === Calculate features =================================================
    cv::Mat bbImg =
        bgImg(cv::Range(stats.at<int>(bigBBId, cv::CC_STAT_TOP),
                        stats.at<int>(bigBBId, cv::CC_STAT_HEIGHT) +
                            stats.at<int>(bigBBId, cv::CC_STAT_TOP)),
              cv::Range(stats.at<int>(bigBBId, cv::CC_STAT_LEFT),
                        stats.at<int>(bigBBId, cv::CC_STAT_WIDTH) +
                            stats.at<int>(bigBBId, cv::CC_STAT_LEFT)));

    double areaContour = cv::contourArea(contours[bigContId], false);

    double solidity =
        areaContour / (stats.at<int>(bigBBId, cv::CC_STAT_WIDTH) *
                       stats.at<int>(bigBBId, cv::CC_STAT_HEIGHT));

    double relCentrX = (centroids.at<float>(bigBBId, 0) -
                        stats.at<int>(bigBBId, cv::CC_STAT_LEFT)) /
                       stats.at<int>(bigBBId, cv::CC_STAT_WIDTH);
    // This relative centroid is inverted, however it was trained as such
    double relCentrY = (centroids.at<float>(bigBBId, 1) -
                        stats.at<int>(bigBBId, cv::CC_STAT_TOP)) /
                       stats.at<int>(bigBBId, cv::CC_STAT_HEIGHT);

    double perimeter = cv::arcLength(contours[bigContId], true);

    double circularity = 4 * PI / pow(perimeter, 2);

    int nObjects =
        cv::connectedComponentsWithStats(bbImg, labels, stats, centroids);
    int nHoles =
        cv::connectedComponentsWithStats(255 - bbImg, labels, stats, centroids);
    double euler = nObjects - nHoles;

    std::vector<cv::Point> hull;
    cv::convexHull(cv::Mat(contours[bigContId]), hull);
    double hullPerim = cv::arcLength(hull, true);
    double hullArea  = cv::contourArea(hull);

    // === Calculate cost =====================================================

    double features[] = {solidity,    relCentrX, relCentrY, perimeter,
                         circularity, euler,     hullPerim, hullArea};
    double output[1];

    std::string filename = "./model.txt";
    auto booster = LightGBM::Boosting::CreateBoosting("gbdt", filename.c_str());

    auto early_stop = CreatePredictionEarlyStopInstance(
        "none", LightGBM::PredictionEarlyStopConfig());
    booster->InitPredict(0, booster->NumberOfTotalModel(), false);
    booster->PredictRaw(features, output, &early_stop);

    return output[0];
}

cv::Mat LGBCostFunction::costImg(const cv::Mat &img) const {
    return this->bgm->bgMask(img);
}
