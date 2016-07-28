#ifndef TASK_SEGMENTATION_STG3_H_
#define TASK_SEGMENTATION_STG3_H_

#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "FileUtils.h"



class TaskSegmentationStg3: public Task {
private:
	cv::Mat* seg_open;
	cv::Mat* seg_nohole;
	::cciutils::SimpleCSVLogger *logger;
	::cciutils::cv::IntermediateResultHandler *iresHandler;

public:
	TaskSegmentationStg3(cv::Mat* seg_nohole, cv::Mat* seg_open, 
		::cciutils::SimpleCSVLogger *logger, ::cciutils::cv::IntermediateResultHandler *iresHandler);

	virtual ~TaskSegmentationStg3();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_SEGMENTATION_STG3_H_ */
