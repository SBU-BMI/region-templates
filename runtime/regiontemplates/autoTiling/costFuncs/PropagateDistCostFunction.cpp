#include "PropagateDistCostFunction.h"

PropagateDistCostFunction::PropagateDistCostFunction(int bgThr, int dilate, int erode) {
    this->bgm = new ThresholdBGMasker(bgThr, dilate, erode);
}

PropagateDistCostFunction::PropagateDistCostFunction(ThresholdBGMasker* bgm) {
    this->bgm = bgm;
}

// We approximate the cost of an image as:
// cost = expectedIterations * imageSize
// 'expectedIterations' is the number of iterations required 
// by the IWPP propagation algorithm.
// 'imageSize' represents the cost of executing a single iteration.
// We assume that on the worst case, a point can propagate at most
// from the furtherest two points of a single dense region (i.e., 
// the feret diameter). 
// This diameter is approximated by creating a rectangular bounding box 
// on each dense region, and returning the diagonal of said rectangle.
// As such, we assume a direct proportionality between the number of
// iterations and this approximated maximum feret diameter.
// Thus, the final cost is approximated as imgSize * maxDiag
int64_t PropagateDistCostFunction::cost(cv::Mat img) const {
    cv::Mat bgImg = this->bgm->bgMask(img);

    // imwrite("cost-bg.png", bgImg);

    // approximate max feret diameter though a rectangular bounding box
    int64_t maxDist=0;
    int64_t curMax;
	std::vector<std::vector<cv::Point>> contours;
    cv::findContours(bgImg, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    // cv::Mat cont = bgImg.clone();
    // cv::cvtColor(cont, cont, cv::COLOR_GRAY2RGB);

    int curMax2;
    for (std::vector<cv::Point> contour : contours) {
    	cv::Rect_<int64_t> r = cv::boundingRect(contour);
	    
	    // cv::rectangle(cont, 
	    //     cv::Point(r.x,r.y), 
	    //     cv::Point(r.x+r.width,
	    //               r.y+r.height),
	    //     (255,255,255),5);

    	curMax = sqrt(pow(r.height,2) + pow(r.width,2));
    	curMax2 = std::max(r.height, r.width);

    	if (curMax>maxDist) maxDist=curMax;
    }
    std::cout << "[PropagateDistCostFunction] curMax: " << curMax << std::endl;
    std::cout << "[PropagateDistCostFunction] curMax2: " << curMax2 << std::endl;
    // imwrite("cost-cont.png", cont);

    return maxDist * img.cols * img.rows;
}

cv::Mat PropagateDistCostFunction::costImg(cv::Mat img) const {
    return this->bgm->bgMask(img);
}