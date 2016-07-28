#ifndef TASK_SEGMENTATION_STG2_H_
#define TASK_SEGMENTATION_STG2_H_

#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "FileUtils.h"



class TaskSegmentationStg2: public Task {
private:
	int* findCandidateResult;
	cv::Mat* seg_norbc;
	cv::Mat* seg_nohole;
	::cciutils::SimpleCSVLogger *logger;
	::cciutils::cv::IntermediateResultHandler *iresHandler;

public:
	TaskSegmentationStg2(int* findCandidateResult, cv::Mat* seg_norbc, cv::Mat* seg_nohole, 
		::cciutils::SimpleCSVLogger *logger, ::cciutils::cv::IntermediateResultHandler *iresHandler);

	virtual ~TaskSegmentationStg2();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_SEGMENTATION_STG2_H_ */
