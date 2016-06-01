#ifndef TASK_SEGMENTATION_STG5_H_
#define TASK_SEGMENTATION_STG5_H_

#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "FileUtils.h"



class TaskSegmentationStg5: public Task {
private:
	int minSizeSeg;
	int maxSizeSeg;
	cv::Mat* seg_nonoverlap;
	cv::Mat* seg;
	::cciutils::SimpleCSVLogger *logger;
	::cciutils::cv::IntermediateResultHandler *iresHandler;

public:
	TaskSegmentationStg5(int minSizeSeg, int maxSizeSeg, cv::Mat* seg_nonoverlap, cv::Mat* seg, 
		::cciutils::SimpleCSVLogger *logger, ::cciutils::cv::IntermediateResultHandler *iresHandler);

	virtual ~TaskSegmentationStg5();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_SEGMENTATION_STG5_H_ */
