

#ifndef TASK_FEATURES_H_
#define TASK_FEATURES_H_

#include "Task.h"
#include "DenseDataRegion2D.h"
#include "DataRegion2DUnaligned.h"
// for testing only
#include "DataRegionFactory.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "Util.h"
#include "FileUtils.h"
#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"

class TaskFeatures: public Task {
private:
	DenseDataRegion2D* bgr;
	DenseDataRegion2D* mask;
	DataRegion2DUnaligned* features;

public:
	TaskFeatures(DenseDataRegion2D* bgr, DenseDataRegion2D* mask, DataRegion2DUnaligned* features);

	virtual ~TaskFeatures();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_FEATURES_H_ */
