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
	/*std::cout << "dr1->id: "<< dr1->getId() << " version: " << dr1->getVersion() << std::endl;
	std::cout << "dr2->id: "<< dr2->getId() << " version: " << dr2->getVersion() << std::endl;*/

	int diffPixels = 0;
	if(image1.rows == image2.rows && image1.cols == image2.cols && image1.rows > 0 && image1.cols > 0){
			diffPixels = countNonZero(image1 != image2);
	}else{
		diffPixels = max(image1.rows * image1.cols, image2.rows * image2.cols);
	}
	int nonZeroRef = countNonZero(image2);

	std::cout << "diffPixels: "<< diffPixels << " nonZeroRef: "<< nonZeroRef<<std::endl;
	this->diffPixels[0] = diffPixels;
	this->diffPixels[1] = nonZeroRef;
	std::cout << "CompId: "<< compId<<" diffPixels: " << this->diffPixels[0] << " reference: " << this->diffPixels[1] << std::endl;

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Diff Computation time elapsed: "<< t2-t1 << std::endl;
}
