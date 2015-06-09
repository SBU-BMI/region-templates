#include "TaskDiffMask.h"

TaskDiffMask::TaskDiffMask(DenseDataRegion2D* mask0, DenseDataRegion2D* mask1, DenseDataRegion2D* feat0, DenseDataRegion2D* feat1) {
	this->mask0 = mask0;
	this->mask1 = mask1;
	this->feat0 = feat0;
	this->feat1 = feat1;
}

TaskDiffMask::~TaskDiffMask() {
}

bool TaskDiffMask::run(int procType, int tid) {
	uint64_t t1 = Util::ClockGetTimeProfile();

	cv::Mat image1 = this->mask0->getData();
	cv::Mat image2 = this->mask1->getData();
	std::cout << "dr1->id: "<< mask0->getId() << std::endl;
	std::cout << "dr2->id: "<< mask1->getId() << std::endl;

	int square_side = 512;
	if(image1.rows == image2.rows && image1.cols == image2.cols && image1.rows > 0 && image1.cols > 0){
		std::vector<int> diffPercentage;
		for(int i = 0; i < image1.cols; i+=square_side){
			for(int j = 0; j < image1.rows; j+=square_side){
				cv::Mat img1ROI(image1, cv::Rect(i, j, square_side, square_side));
				cv::Mat img2ROI(image2, cv::Rect(i, j, square_side, square_side));
				int diffPixels = countNonZero(img1ROI != img2ROI);
				int nonZero1 = countNonZero(img1ROI);
				int nonZero2 = countNonZero(img2ROI);
				if(nonZero1 > 100){
//					std::cout << "Diff: " << 100*diffPixels/((nonZero1+nonZero2)/2) << " diff: "<< diffPixels<<std::endl;
				//	std::cout << "Diff: " << "i: "<< i << " j:"<<j<< " diff:"<< diffPixels << " image1: "<< nonZero1 << " image2: "<< nonZero2<< std::endl;
					diffPercentage.push_back(100*diffPixels/((nonZero1+nonZero2)/2));
				}
			}
		}
		int avg = 0;
		double std = 0;
		for(int i =0; i < diffPercentage.size(); i++){
			avg += diffPercentage[i];
			std += diffPercentage[i]*diffPercentage[i];
		}
		if(diffPercentage.size() > 0){
			avg /= diffPercentage.size();
			std /= diffPercentage.size();
			std -= (avg * avg);// variance
	//	std::cout << "variance: "<< std << std::endl;
			std = sqrt(std); // std
		}else{
			std::cout <<"DIFFPERCENTAGE=0" <<std::endl;
		}
		std::cout << "Diff: avg: "<< avg << " std: " << std << std::endl;

		cv::Mat abs0 = cv::abs(feat0->getData());
		cv::Mat abs1 = cv::abs(feat1->getData());
		cv::Mat max = cv::max(abs0, abs1);
		cv::Mat res;
		cv::absdiff(abs0, abs1, res);
		res /= max;
		res *=100;
		std::cout << "Features diff: " << std::endl;
		for(int i = 0; i < res.cols; i++){
			std::cout << ((float*)(res.data))[i]<<" ";
		}
		std::cout << std::endl;




//	}
//	if(image1.rows > 0 && image2.rows > 0){
//		int diffPixels = countNonZero(image1 != image2);
//		int nonZero1 = countNonZero(image1);
//		int nonZero2 = countNonZero(image2);
//		std::cout << "Diff: "<< diffPixels << " image1: "<< nonZero1 << " image2: "<< nonZero2<< std::endl;
	}else{
		std::cout << "Not Computing Diff! One or both masks are NULL!" << std::endl;
	}
	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Diff Computation time elapsed: "<< t2-t1 << std::endl;
}
