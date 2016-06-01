

#ifndef GENERAL_TASK_H_
#define GENERAL_TASK_H_

#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "FileUtils.h"



class GeneralTask: public Task {
private:
	DenseDataRegion2D* bgr;
	DenseDataRegion2D* mask;

	unsigned char blue, green, red;
	double T1, T2;
	unsigned char G1, G2;
	int minSize, maxSize, minSizePl, minSizeSeg, maxSizeSeg;
	int fillHolesConnectivity, reconConnectivity, watershedConnectivity;

public:
	GeneralTask(DenseDataRegion2D* bgr, DenseDataRegion2D* mask, unsigned char blue, unsigned char green, unsigned char red, double T1, double T2, unsigned char G1, unsigned char G2, int minSize, int maxSize, int minSizePl, int minSizeSeg, int maxSizeSeg, int fillHolesConnectivity, int reconConnectivity, int watershedConnectivity);

	virtual ~GeneralTask();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* GENERAL_TASK_H_ */
