/*
 * Segmentation.h
 *
 *  GENERATED CODE
 *  DO NOT CHANGE IT MANUALLY!!!!!
 */

#ifndef Segmentation_H_
#define Segmentation_H_

#include "RTPipelineComponentBase.h"
#include "Util.h"
#include "FileUtils.h"
#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
// #include "opencv2/gpu/gpu.hpp" // old opencv 2.4
#ifdef WITH_CUDA
#include "opencv2/cudaarithm.hpp" // new opencv 3.4.1
#endif
#include "HistologicalEntities.h"

// PipelineComponent header
class Segmentation : public RTPipelineComponentBase {

private:
	// data region id
	// IMPORTANT: this need to be set during the creation of this object
	int dr_id;

public:
	Segmentation();
	virtual ~Segmentation();

	void set_dr_id(int id) {dr_id = id;};

	int run();
};

// Task header
class TaskSegmentation: public Task {
private:

	// data regions
	DenseDataRegion2D* inputImage_temp;
	DenseDataRegion2D* outMask_temp;


	// all other variables
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	double T1;
	double T2;
	unsigned char G1;
	int minSize;
	int maxSize;
	unsigned char G2;
	int minSizePl;
	int minSizeSeg;
	int maxSizeSeg;
	int fillHolesConnectivity;
	int reconConnectivity;
	int watershedConnectivity;


public:
	TaskSegmentation(DenseDataRegion2D* inputImage_temp, DenseDataRegion2D* outMask_temp, unsigned char blue, unsigned char green, unsigned char red, double T1, double T2, unsigned char G1, int minSize, int maxSize, unsigned char G2, int minSizePl, int minSizeSeg, int maxSizeSeg, int fillHolesConnectivity, int reconConnectivity, int watershedConnectivity);

	virtual ~TaskSegmentation();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* Segmentation_H_ */
