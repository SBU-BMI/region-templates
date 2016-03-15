#include "TaskDiffMask.h"

TaskDiffMask::TaskDiffMask(DenseDataRegion2D* dr1, DenseDataRegion2D* dr2, int *diffPixels) {
	this->dr1 = dr1;
	this->dr2 = dr2;
	this->diffPixels = diffPixels;
}

TaskDiffMask::~TaskDiffMask() {
	if(dr1 != NULL) delete dr1;
	if(dr2 != NULL) delete dr2;
}

bool TaskDiffMask::run(int procType, int tid) {
	uint64_t t1 = Util::ClockGetTimeProfile();
	int compId = diffPixels[0];
	cv::Mat image1 = this->dr1->getData();
	cv::Mat image2 = this->dr2->getData();
/*	std::cout << "dr1->id: "<< dr1->getId() << " version: " << dr1->getVersion() << std::endl;
	std::cout << "dr2->id: "<< dr2->getId() << " version: " << dr2->getVersion() << std::endl;*/
	int nonZeroRef = 0, diffPixels = 0, foregrond1 = 0, foregrond2 = 0, commonPixels = 0;
	double DICE = 0.0;
	if(image1.rows == image2.rows && image1.cols == image2.cols && image1.rows > 0 && image1.cols > 0){
		cv::Mat img1mask = image1 > 0;	
		cv::Mat img2mask = image2 > 0;

		cv::Mat common = cv::Mat::zeros(img1mask.rows, img1mask.cols, img1mask.type());
		common = ((img1mask > 0 & img2mask) > 0);
		commonPixels = cv::countNonZero(common);

		cv::Mat firstNotSecond = (img1mask-img2mask) > 0;

		// In second, and not first
		cv::Mat secondNotFirst = (img2mask-img1mask) > 0;

		diffPixels = countNonZero(firstNotSecond) + countNonZero(secondNotFirst);
		nonZeroRef = countNonZero(img2mask);

		foregrond1 = countNonZero(img1mask >0);
		foregrond2 = countNonZero(img2mask >0);
		DICE =  ((double)2*commonPixels)/(foregrond1+foregrond2);
	}else{
		std::cout << "WARNNING: images have different sizes in the comparison compoent."<< std::endl; 
		diffPixels = max(image1.rows * image1.cols, image2.rows * image2.cols);
		nonZeroRef = countNonZero(image1);
	}


	std::cout << "diffPixels: "<< diffPixels << " nonZeroRef: "<< nonZeroRef<<" DICE: " << DICE << std::endl;
	this->diffPixels[0] = diffPixels;
	this->diffPixels[1] = nonZeroRef;
	this->diffPixels[2] = foregrond1+foregrond2;
	this->diffPixels[3] = commonPixels;
	std::cout << "CompId: "<< compId<<" diffPixels: " << this->diffPixels[0] << " reference: " << this->diffPixels[1] << std::endl;

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Diff Computation time elapsed: "<< t2-t1 << std::endl;
}
