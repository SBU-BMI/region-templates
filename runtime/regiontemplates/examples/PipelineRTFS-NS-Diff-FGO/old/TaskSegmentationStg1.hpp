#ifndef TASK_SEGMENTATION_STG1_H_
#define TASK_SEGMENTATION_STG1_H_

#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "FileUtils.h"



class TaskSegmentationStg1: public Task {
private:
	cv::Mat img;

	unsigned char blue, green, red;
	double T1, T2;
	unsigned char G1, G2;
	int minSize, maxSize;

	int fillHolesConnectivity, reconConnectivity;
	int* findCandidateResult;
	cv::Mat* seg_norbc;
	
	::cciutils::SimpleCSVLogger *logger;
	::cciutils::cv::IntermediateResultHandler *iresHandler;

public:
	TaskSegmentationStg1(DenseDataRegion2D* bgr, unsigned char blue, unsigned char green, unsigned char red, 
		double T1, double T2, unsigned char G1, int minSize, int maxSize, unsigned char G2, 
		int fillHolesConnectivity, int reconConnectivity, int* findCandidateResult, 
		cv::Mat* seg_norbc, ::cciutils::SimpleCSVLogger *logger, ::cciutils::cv::IntermediateResultHandler *iresHandler);

	virtual ~TaskSegmentationStg1();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_SEGMENTATION_STG1_H_ */
