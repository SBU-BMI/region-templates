

#ifndef TASK_SEGMENTATION_H_
#define TASK_SEGMENTATION_H_

#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
// #include "opencv2/gpu/gpu.hpp" // old opencv 2.4
// #include "opencv2/cudaarithm.hpp" // new opencv 3.4.1
#include "HistologicalEntities.h"
#include "Util.h"
#include "FileUtils.h"

class TaskSegmentation: public Task {
private:
	DenseDataRegion2D* bgr;
	DenseDataRegion2D* mask;

	unsigned char blue, green, red;
	double T1, T2;
	unsigned char G1, G2;
	int minSize, maxSize, minSizePl, minSizeSeg, maxSizeSeg;
	int fillHolesConnectivity, reconConnectivity, watershedConnectivity;

public:
	TaskSegmentation(DenseDataRegion2D* bgr, DenseDataRegion2D* mask, unsigned char blue, unsigned char green, unsigned char red, double T1, double T2, unsigned char G1, unsigned char G2, int minSize, int maxSize, int minSizePl, int minSizeSeg, int maxSizeSeg, int fillHolesConnectivity, int reconConnectivity, int watershedConnectivity);

	virtual ~TaskSegmentation();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_SEGMENTATION_H_ */
