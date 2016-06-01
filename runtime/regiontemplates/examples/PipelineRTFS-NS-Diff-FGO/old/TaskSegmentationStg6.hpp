#ifndef TASK_SEGMENTATION_STG6_H_
#define TASK_SEGMENTATION_STG6_H_

#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "FileUtils.h"



class TaskSegmentationStg6: public Task {
private:
	DenseDataRegion2D* output;
	cv::Mat* seg;
	int fillHolesConnectivity;
	::cciutils::SimpleCSVLogger *logger;
	::cciutils::cv::IntermediateResultHandler *iresHandler;

public:
	TaskSegmentationStg6(cv::Mat* seg, DenseDataRegion2D* output, int fillHolesConnectivity, ::cciutils::SimpleCSVLogger *logger, ::cciutils::cv::IntermediateResultHandler *iresHandler);

	virtual ~TaskSegmentationStg6();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_SEGMENTATION_STG6_H_ */
