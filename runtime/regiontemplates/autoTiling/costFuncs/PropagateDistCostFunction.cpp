#include "PropagateDistCostFunction.h"

// PropagateDistCostFunction::PropagateDistCostFunction(int e1, int d1, int e2, int d2) {
PropagateDistCostFunction::PropagateDistCostFunction(int e1, int e2, int d) {
	this->e1=e1;
	this->d1=d;
	this->e2=e2;
	this->d2=d;
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
    ColorThresholdBGMasker bgmDense(this->e1, this->d1);
    cv::Mat denseMask = bgmDense.bgMask(img);

    // approximate max feret diameter of dense mask though a rectangular bounding box
    int64_t maxDist=0;
    int64_t curMax;
	std::vector<std::vector<cv::Point>> contours;
    cv::findContours(denseMask, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    #ifdef DEBUG
    cv::Mat cont = denseMask.clone();
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


    // sparse mask
    ColorThresholdBGMasker bgmSparse(this->e2, this->d2);
    cv::Mat sparseMask = bgmSparse.bgMask(img);


    long final = maxDist * cv::sum(sparseMask)[0];


    #ifdef DEBUG
    imwrite("cost-cont.png", cont);
    imwrite("cost-cont2.png", sparseMask);

    setlocale(LC_NUMERIC, "pt_BR.utf-8");
    char c_cost[50];
    sprintf(c_cost, "%'2ld", final);

    std::cout << "[PropagateDistCostFunction] maxDist: " 
    	<< maxDist << std::endl;
    std::cout << "[PropagateDistCostFunction] final: " 
    	<< c_cost << std::endl;
    #endif

    return final;
}

cv::Mat PropagateDistCostFunction::costImg(cv::Mat img) const {
    ColorThresholdBGMasker bgmDense(this->e1, this->d1);
    return bgmDense.bgMask(img);
}