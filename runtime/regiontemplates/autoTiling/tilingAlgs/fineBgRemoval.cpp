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
                int xi, xo, yi, yo;
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

        // Generation of Dense partitions
        std::list<rect_t> denseRects;
        auto              bgRects = toMyRectT(bestRects);
        denseRects                = generateBackground(bgRects, curImg.cols - 1,
                                                       curImg.rows - 1, false);

        // Attempt to reduce dense partitions generated
        int                 minLen = 50;
        std::vector<rect_t> denseRectsVect(denseRects.begin(),
                                           denseRects.end());

        // Ignoring first and last elements for ease of iterations
        bool changed;
        do {
            changed = false;
            for (auto curRIt = std::next(denseRectsVect.begin());
                 curRIt != std::prev(denseRectsVect.end()); curRIt++) {
                rect_t curR = *curRIt;
                // If the height is too short
                if (curR.yo - curR.yi + 1 < minLen) {
                    // Get neighbors
                    rect_t upperR = *std::prev(curRIt);
                    rect_t lowerR = *std::next(curRIt);

                    // Get amount of added area for each expansion
                    int expandUpCost = (curR.yo - curR.yi + 1) *
                                       (std::abs(curR.xi - upperR.xi) +
                                        std::abs(curR.xo - upperR.xo));
                    int expandDownCost = (curR.yo - curR.yi + 1) *
                                         (std::abs(curR.xi - lowerR.xi) +
                                          std::abs(curR.xo - lowerR.xo));

                    // Expand one of the compared partitions
                    if (expandUpCost < expandDownCost) {
                        std::prev(curRIt)->yo = curR.yo;
                        if (curR.xi == upperR.xi)
                            std::prev(curRIt)->xo =
                                std::max(curR.xo, upperR.xo);
                        else
                            std::prev(curRIt)->xi =
                                std::min(curR.xi, upperR.xi);
                    } else { // Expand down
                        std::next(curRIt)->yi = curR.yi;
                        if (curR.xi == lowerR.xi)
                            std::next(curRIt)->xo =
                                std::max(curR.xo, lowerR.xo);
                        else
                            std::next(curRIt)->xi =
                                std::min(curR.xi, lowerR.xi);
                    }

                    // Remove the cur expanded partition and break since
                    // iterator is not valid anymore
                    denseRectsVect.erase(curRIt);
                    changed = true;
                    break;
                }

                // If the width is too short
                if (curR.xo - curR.xi + 1 < minLen) {
                    // Get neighbors
                    rect_t leftR  = *std::prev(curRIt);
                    rect_t rightR = *std::next(curRIt);

                    // Get amount of added area for each expansion
                    int expandLeftCost = (curR.xo - curR.xi + 1) *
                                         (std::abs(curR.yi - leftR.yi) +
                                          std::abs(curR.yo - leftR.yo));
                    int expandRigthCost = (curR.xo - curR.xi + 1) *
                                          (std::abs(curR.yi - rightR.yi) +
                                           std::abs(curR.yo - rightR.yo));

                    // Expand one of the compared partitions
                    if (expandLeftCost < expandRigthCost) {
                        std::prev(curRIt)->xo = curR.xo;
                        if (curR.yi == leftR.yi)
                            std::prev(curRIt)->yo = std::max(curR.yo, leftR.yo);
                        else
                            std::prev(curRIt)->yi = std::min(curR.yi, leftR.yi);
                    } else { // Expand right
                        std::next(curRIt)->xi = curR.xi;
                        if (curR.yi == rightR.yi)
                            std::next(curRIt)->yo =
                                std::max(curR.yo, rightR.yo);
                        else
                            std::next(curRIt)->yi =
                                std::min(curR.yi, rightR.yi);
                    }

                    // Remove the cur expanded partition and break since
                    // iterator is not valid anymore
                    denseRectsVect.erase(curRIt);
                    changed = true;
                    break;
                }
            }
        } while (changed);

        cv::Mat outb = cv::Mat::zeros(curImg.rows, curImg.cols, CV_8U);
        cv::drawContours(outb, contours, contours.size() - 1, cv::Scalar(255));
        cv::Mat outd = cv::Mat::zeros(curImg.rows, curImg.cols, CV_8U);
        cv::drawContours(outd, contours, contours.size() - 1, cv::Scalar(255));
        cv::Mat outd2 = cv::Mat::zeros(curImg.rows, curImg.cols, CV_8U);
        cv::drawContours(outd2, contours, contours.size() - 1, cv::Scalar(255));
        for (auto bestRect : bestRects) {
            std::cout << "BG: " << bestRect.x << "-"
                      << (bestRect.x + bestRect.width - 1) << ", " << bestRect.y
                      << "-" << (bestRect.y + bestRect.height - 1) << "\n";

            cv::rectangle(outb, cv::Point(bestRect.x, bestRect.y),
                          cv::Point(bestRect.x + bestRect.width + 1,
                                    bestRect.y + bestRect.height + 1),
                          cv::Scalar(255), 3);
        }

        for (auto r : denseRects) {
            std::cout << "dense: " << r.xi << ":" << r.xo << ", " << r.yi << ":"
                      << r.yo << "\n";
            cv::rectangle(outd, cv::Point(r.xi, r.yi), cv::Point(r.xo, r.yo),
                          cv::Scalar(255), 3);
        }

        for (auto r : denseRectsVect) {
            std::cout << "dense: " << r.xi << ":" << r.xo << ", " << r.yi << ":"
                      << r.yo << "\n";
            cv::rectangle(outd2, cv::Point(r.xi, r.yi), cv::Point(r.xo, r.yo),
                          cv::Scalar(255), 3);
        }

        cv::imwrite("bg.png", outb);
        cv::imwrite("denseBefore.png", outd);
        cv::imwrite("denseAfter.png", outd2);
    }

    exit(88);
}
