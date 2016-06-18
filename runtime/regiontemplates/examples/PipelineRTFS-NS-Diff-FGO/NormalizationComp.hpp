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
#include "opencv2/gpu/gpu.hpp"
#include "Normalization.h"

// PipelineComponent header
class NormalizationComp : public RTPipelineComponentBase {

private:
	// data region id
	// IMPORTANT: this need to be set during the creation of this object
	int workflow_id;

public:
	NormalizationComp();
	virtual ~NormalizationComp();

	void set_workflow_id(int id) {workflow_id = id;};

	int run();
};

// Task header
class TaskNormalizationComp: public Task {
private:

	// data regions
	DenseDataRegion2D* input_img_temp;
	DenseDataRegion2D* normalized_rt_temp;


	// all other variables
	float* target_mean;
	float* target_std;


public:
	TaskNormalizationComp(DenseDataRegion2D* input_img_temp, DenseDataRegion2D* normalized_rt_temp, float* target_mean, float* target_std);

	virtual ~TaskNormalizationComp();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* NormalizationComp_H_ */
