

#ifndef TASK_SEGMENTATION_H_
#define TASK_SEGMENTATION_H_

#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
/*#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"*/
#include "Util.h"
#include "FileUtils.h"






class TaskSegmentation: public Task {
private:
	DenseDataRegion2D* bgr;
	DenseDataRegion2D* mask;

public:

    float otsuRatio;
    double curvatureWeight;
    float sizeThld;
    float sizeUpperThld;
    double mpp;
    float msKernel;
    int levelsetNumberOfIteration;

	TaskSegmentation(DenseDataRegion2D* bgr, DenseDataRegion2D* mask);

	virtual ~TaskSegmentation();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_SEGMENTATION_H_ */
