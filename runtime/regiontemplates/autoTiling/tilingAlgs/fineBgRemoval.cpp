#include "fineBgRemoval.h"
#include "denseFromBG.h"
#include <vector>

void insertSquareContour(std::vector<std::list<int>> &hCoords,
                         std::vector<std::list<int>> &vCoords,
                         cv::Rect_<uint64_t>          rect) {
    int xi = rect.x;
    int xo = rect.x + rect.width - 1;
    int yi = rect.y;
    int yo = rect.y + rect.height - 1;

    // Insert the two points of the vertical line (begin and end)
    for (int i = xi; i <= xo; i++) {
        vCoords[i].emplace_back(yi);
        vCoords[i].emplace_back(yo);
    }

    // Insert the two points of the horizontal line (begin and end)
    for (int i = yi; i <= yo; i++) {
        hCoords[i].emplace_back(xi);
        hCoords[i].emplace_back(xo);
    }
}

std::list<rect_t> fineBgRemoval(cv::Mat img, std::list<rect_t> tiles) {

    // Iterate through all given partitions
    for (auto tile = tiles.begin(); tile != tiles.end(); tile++) {
        // Get outermost contour from dense image
        cv::Mat curImg =
            img(cv::Range(tile->yi, tile->yo), cv::Range(tile->xi, tile->xo));
        auto M = cv::Mat::ones(7, 7, CV_8U);
        cv::morphologyEx(curImg, curImg, cv::MORPH_GRADIENT, M);
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i>              hierarchy;
        cv::findContours(curImg, contours, hierarchy, CV_RETR_EXTERNAL,
                         CV_CHAIN_APPROX_NONE);
        std::vector<cv::Point> outterContour = contours[contours.size() - 1];

        // Generate list of lined coordinates. Rects versions are for BG
        // rectangles added later.
        // horizontal lines
        std::vector<std::list<int>> hCoords(curImg.rows, std::list<int>());
        std::vector<std::list<int>> hCoordsRects(curImg.rows, std::list<int>());
        // vertical lines
        std::vector<std::list<int>> vCoords(curImg.cols, std::list<int>());
        std::vector<std::list<int>> vCoordsRects(curImg.cols, std::list<int>());

        // Fill values of initial contour line
        for (cv::Point p : outterContour) {
            hCoords[p.y].emplace_back(p.x);
            vCoords[p.x].emplace_back(p.y);
        }

        // Extract multiple BG rectangles
        std::list<cv::Rect_<uint64_t>> bestRects;
        for (int i = 0; i < 8; i++) {

            // Find a rectangular BG region to remove
            int                 bestArea = 0;
            cv::Rect_<uint64_t> bestRect;
            for (cv::Point p : outterContour) {
                // Check upper/lower partition
                bool upper = true;
                bool lower = true;
                for (int ys : vCoords[p.x]) {
                    if (ys == p.y)
                        continue;

                    if (ys < p.y)
                        upper = false;
                    if (ys > p.y)
                        lower = false;
                }

                // Check left/right partition
                bool left  = true;
                bool right = true;
                for (int xs : hCoords[p.y]) {
                    if (xs == p.x)
                        continue;

                    if (xs < p.x)
                        left = false;
                    if (xs > p.x)
                        right = false;
                }

                // Check if one coordinate is impossible (point between two
                // other points on vertical or horizontal axis)
                if ((!upper && !lower) || (!left && !right))
                    continue;

                // Also do nothing for the case of a peak point
                if ((upper && lower) || (left && right))
                    continue;

                // Generate background partitions if there were no other
                // partitions in the way (may overlap)
                int xi = 0, xo = curImg.cols, yi = 0, yo = curImg.rows;
                if (left) {
                    xi = 0;
                    xo = p.x;
                } else { // right
                    xi = p.x;
                    xo = curImg.cols - 1;
                }
                if (upper) {
                    yi = 0;
                    yo = p.y;
                } else { // lower
                    yi = p.y;
                    yo = curImg.rows - 1;
                }

                // Update x coordinates for BG partition to remove overlapping
                for (auto rr : hCoordsRects[p.y]) {
                    if (left) {
                        if (rr <= p.x && rr >= xi)
                            xi = rr;
                    } else { // right
                        if (rr >= p.x && rr <= xo)
                            xo = rr;
                    }
                }

                // Update y coordinates for BG partition to remove overlapping
                for (auto rr : vCoordsRects[p.x]) {
                    if (upper) {
                        if (rr <= p.y && rr >= yi)
                            yi = rr;
                    } else { // lower
                        if (rr >= p.y && rr <= yo) {
                            yo = rr;
                        }
                    }
                }

                // Calculate height and width of new background partition
                int  width   = xo - xi + 1;
                int  height  = yo - yi + 1;
                long curArea = width * height;

                // Update best point with largest BG area
                if (bestArea < curArea) {
                    bestArea = curArea;
                    bestRect = {xi, yi, width, height};
                }
            }

            // Insert new BG partition on best list and on Coord vectors
            insertSquareContour(hCoordsRects, vCoordsRects, bestRect);
            bestRects.emplace_back(bestRect);
        }

        cv::Mat out = cv::Mat::zeros(curImg.rows, curImg.cols, CV_8U);
        for (auto bestRect : bestRects) {
            // std::cout << "best point " << bestPoint.x << ", " << bestPoint.y
            //           << "\n";
            std::cout << "best rect " << bestRect.x << "-" << bestRect.width
                      << ", " << bestRect.y << "-" << bestRect.height << "\n";

            cv::rectangle(out, cv::Point(bestRect.x, bestRect.y),
                          cv::Point(bestRect.x + bestRect.width,
                                    bestRect.y + bestRect.height),
                          cv::Scalar(255), 3);
            cv::drawContours(out, contours, contours.size() - 1,
                             cv::Scalar(255));
        }

        // Generation of Dense partitions
        std::list<rect_t> denseRects;
        auto              bgRects = toMyRectT(bestRects);
        generateBackground(bgRects, denseRects, out.cols, out.rows);

        for (auto r : denseRects) {
            cv::rectangle(out, cv::Point(r.xi, r.yi), cv::Point(r.xo, r.yo),
                          cv::Scalar(255), 3);
            cv::drawContours(out, contours, contours.size() - 1,
                             cv::Scalar(255));
        }

        cv::imwrite("initialdense.png", out);
    }

    exit(88);
}
