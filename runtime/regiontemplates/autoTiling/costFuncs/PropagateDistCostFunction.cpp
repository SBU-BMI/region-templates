#include "PropagateDistCostFunction.h"

// template <typename T>
// ThresholdBGCostFunction<T>::ThresholdBGCostFunction(int bgThr, int dilate, int erode) {
PropagateDistCostFunction::PropagateDistCostFunction(int bgThr, int dilate, int erode) {
    this->bgm = new ThresholdBGMasker(bgThr, dilate, erode);
}

// template <typename T>
// ThresholdBGCostFunction<T>::ThresholdBGCostFunction(ThresholdBGMasker bgm) {
PropagateDistCostFunction::PropagateDistCostFunction(BGMasker* bgm) {
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
    // dense mask
    cv::Mat thrsMask = this->bgm->bgMask(img);

    // approximate max feret diameter of the mask though a rectangular bounding box
    int64_t maxDist=0;
    int64_t curMax;
	std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thrsMask, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    #ifdef DEBUG
    cv::Mat cont = thrsMask.clone();
    cv::cvtColor(cont, cont, cv::COLOR_GRAY2RGB);
    #endif

    for (std::vector<cv::Point> contour : contours) {
    	cv::Rect_<int64_t> r = cv::boundingRect(contour);
	    #ifdef DEBUG
	    cv::rectangle(cont, 
	        cv::Point(r.x,r.y), 
	        cv::Point(r.x+r.width,
	                  r.y+r.height),
	        (255,255,255),5);
    	#endif

    	curMax = sqrt(pow(r.height,2) + pow(r.width,2));

    	if (curMax>maxDist) maxDist=curMax;
    }

    int64_t thrsSum = cv::sum(thrsMask)[0];

    // int64_t final = maxDist * thrsSum * img.cols * img.rows;
    // int64_t final = maxDist * img.cols * img.rows;
    int64_t final = thrsSum * img.cols * img.rows;

    return final;
}

cv::Mat PropagateDistCostFunction::costImg(cv::Mat img) const {
    return this->bgm->bgMask(img);
}