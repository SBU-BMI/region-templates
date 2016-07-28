#ifndef TASK_SEGMENTATION_STG4_H_
#define TASK_SEGMENTATION_STG4_H_

#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "FileUtils.h"



class TaskSegmentationStg4: public Task {
private:
	const cv::Mat* img;
	int minSizePl;
	int watershedConnectivity;
	cv::Mat* seg_open;
	cv::Mat* seg_nonoverlap;
	::cciutils::SimpleCSVLogger *logger;
	::cciutils::cv::IntermediateResultHandler *iresHandler;

public:
	TaskSegmentationStg4(const cv::Mat* img, int minSizePl, int watershedConnectivity, 
		cv::Mat* seg_open, cv::Mat* seg_nonoverlap, ::cciutils::SimpleCSVLogger *logger, 
		::cciutils::cv::IntermediateResultHandler *iresHandler);

	virtual ~TaskSegmentationStg4();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_SEGMENTATION_STG4_H_ */
