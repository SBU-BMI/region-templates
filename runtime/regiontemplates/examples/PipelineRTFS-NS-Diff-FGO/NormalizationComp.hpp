/*
 * NormalizationComp.h
 *
 *  GENERATED CODE
 *  DO NOT CHANGE IT MANUALLY!!!!!
 */

#ifndef NormalizationComp_H_
#define NormalizationComp_H_

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
#include "Normalization.h"

// PipelineComponent header
class NormalizationComp : public RTPipelineComponentBase {

private:
	// data region id
	// IMPORTANT: this need to be set during the creation of this object
	int dr_id;

public:
	NormalizationComp();
	virtual ~NormalizationComp();

	void set_dr_id(int id) {dr_id = id;};

	int run();
};

// Task header
class TaskNormalizationComp: public Task {
private:

	// data regions
	DenseDataRegion2D* inputImage_temp;
	DenseDataRegion2D* normalizedImg_temp;


	// all other variables
	float* targetMean;
	float* targetStd;


public:
	TaskNormalizationComp(DenseDataRegion2D* inputImage_temp, DenseDataRegion2D* normalizedImg_temp, float* targetMean, float* targetStd);

	virtual ~TaskNormalizationComp();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* NormalizationComp_H_ */
