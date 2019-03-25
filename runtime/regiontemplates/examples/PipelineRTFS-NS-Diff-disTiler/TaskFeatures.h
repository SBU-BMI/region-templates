

#ifndef TASK_FEATURES_H_
#define TASK_FEATURES_H_

#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
// #include "opencv2/gpu/gpu.hpp" // old opencv 2.4
#include "opencv2/cudaarithm.hpp" // new opencv 3.4.1
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "FileUtils.h"
#include "ObjFeatures.h"
#include "ConnComponents.h"

class TaskFeatures: public Task {
private:
	DenseDataRegion2D* bgr;
	DenseDataRegion2D* mask;

public:
	TaskFeatures(DenseDataRegion2D* bgr, DenseDataRegion2D* mask);

	virtual ~TaskFeatures();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_FEATURES_H_ */
