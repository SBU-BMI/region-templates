#include <string>

#include <opencv/cv.hpp>

#include <LightGBM/boosting.h>
#include <LightGBM/prediction_early_stop.h>

#include "openslide.h"

#define PI 3.1415

// Extracts a roi from osr described by r, outputting it into thisTile
void osrRegionToCVMat(openslide_t *osr, cv::Rect_<int64_t> r, int level,
                      cv::Mat &thisTile) {

    uint32_t *osrRegion = new uint32_t[r.width * r.height];
    openslide_read_region(osr, osrRegion, r.x, r.y, level, r.width, r.height);

    cv::Mat rgbaTile = cv::Mat(r.height, r.width, CV_8UC4, osrRegion);
    thisTile         = cv::Mat(r.height, r.width, CV_8UC3, cv::Scalar(0, 0, 0));
    int64_t numOfPixelPerTile = thisTile.total();

    cv::cvtColor(rgbaTile, thisTile, CV_RGBA2RGB);

    delete[] osrRegion;

    return;
}

int main(int argc, char const *argv[]) {

    std::string filename = "./model.txt";
    auto booster = LightGBM::Boosting::CreateBoosting("gbdt", filename.c_str());

    // Opens svs input file
    auto osr = openslide_open(
        "/mnt/ccecd635-78a2-4fcd-b042-3789ffdd7559/all-imgs/"
        "TCGA-BR-8285-01A-01-TS1.e63a0d5f-8e84-41f0-a75a-68467b862a51.svs");

    // Opens smallest image as a cv mat
    int64_t osrMinLevel = openslide_get_level_count(osr) - 1; // last level
    int64_t osrMaxLevel = 0;
    int64_t w, h;
    openslide_get_level_dimensions(osr, osrMinLevel, &w, &h);
    cv::Rect_<int64_t> roi(10000, 40000, 10000, 8000);
    cv::Mat            img;
    osrRegionToCVMat(osr, roi, osrMaxLevel, img);

    // Close .svs file
    openslide_close(osr);

    cv::Mat imgMask;
    cv::cvtColor(img, imgMask, CV_RGB2GRAY);
    cv::threshold(imgMask, imgMask, 128, 255,
                  cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    cv::Mat labels, stats, centroids;

    int  erosion_size = 1;
    auto element      = cv::getStructuringElement(
             cv::MORPH_ELLIPSE,
             cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1));
    cv::erode(imgMask, imgMask, element);
    int dilate_size = 5;
    element         = cv::getStructuringElement(
                cv::MORPH_ELLIPSE, cv::Size(2 * dilate_size + 1, 2 * dilate_size + 1));
    cv::dilate(imgMask, imgMask, element);
    erosion_size = 4;
    element      = cv::getStructuringElement(
             cv::MORPH_ELLIPSE,
             cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1));
    cv::erode(imgMask, imgMask, element);

    int nLabels =
        cv::connectedComponentsWithStats(imgMask, labels, stats, centroids);

    cv::Mat                             hierarchy;
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(imgMask, contours, hierarchy, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);

    int  bigContId;
    long maxArea = 0;
    for (int i = 0; i < contours.size(); i++) {
        auto area = cv::contourArea(contours[i], false);
        if (area > maxArea) {
            maxArea   = area;
            bigContId = i;
        }
    }

    int bigBBId;
    maxArea = 0;
    for (int i = 0; i < nLabels; i++) {
        auto area = stats.at<int>(i, cv::CC_STAT_AREA);
        if (area > maxArea) {
            maxArea = area;
            bigBBId = i;
        }
    }

    cv::Mat bbImg =
        imgMask(cv::Range(stats.at<int>(bigBBId, cv::CC_STAT_TOP),
                          stats.at<int>(bigBBId, cv::CC_STAT_HEIGHT) -
                              stats.at<int>(bigBBId, cv::CC_STAT_TOP)),
                cv::Range(stats.at<int>(bigBBId, cv::CC_STAT_LEFT),
                          stats.at<int>(bigBBId, cv::CC_STAT_WIDTH) -
                              stats.at<int>(bigBBId, cv::CC_STAT_LEFT)));

    // cv::cvtColor(bbImg, bbImg, CV_GRAY2RGB);
    // cv::drawContours(bbImg, contours, cv::Scalar(255, 0, 0), 5);
    // cv::imwrite("test.tiff", bbImg);

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

    double features[] = {solidity,    relCentrX, relCentrY, perimeter,
                         circularity, euler,     hullPerim, hullArea};

    // double features[] = {0.99448228725896,        0.4987962037019174,
    //                      -0.5465914044851826,     1437.0710676908493,
    //                      0.000006084898975177666, 352.75,
    //                      1435.6301460266113,      129508.0};
    double output[1];

    for (int i = 0; i < 8; i++)
        std::cout << features[i] << ", ";
    std::cout << "\n";

    auto early_stop = CreatePredictionEarlyStopInstance(
        "none", LightGBM::PredictionEarlyStopConfig());
    booster->InitPredict(0, booster->NumberOfTotalModel(), false);
    booster->PredictRaw(features, output, &early_stop);
    std::cout << output[0] << "\n";

    return 0;
}

// solidity rel-centr-x rel-centr-y perimeter circularity
// euler hull-perimeter hul-area
