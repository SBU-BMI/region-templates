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
#include "MergableStage.hpp"
#include "ReusableTask.hpp"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"

// PipelineComponent header
class Segmentation : public RTPipelineComponentBase, public MergableStage {

private:
	// data region id
	// IMPORTANT: this need to be set during the creation of this object
	int workflow_id;

	list<ReusableTask*> mergable_t1;

public:
	Segmentation();
	virtual ~Segmentation();

	void set_workflow_id(int id) {workflow_id = id;};

	void merge(MergableStage &s);

	int run();
};

// Task header
class TaskSegmentation: public ReusableTask {
private:

	// data regions
	DenseDataRegion2D* normalized_rt_temp;
	DenseDataRegion2D* segmented_rt_temp;

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
	TaskSegmentation(){};
	TaskSegmentation(list<ArgumentBase*> args, RegionTemplate* inputRt){};
	TaskSegmentation(DenseDataRegion2D* normalized_rt_temp, DenseDataRegion2D* segmented_rt_temp, unsigned char blue, unsigned char green, unsigned char red, double T1, double T2, unsigned char G1, int minSize, int maxSize, unsigned char G2, int minSizePl, int minSizeSeg, int maxSizeSeg, int fillHolesConnectivity, int reconConnectivity, int watershedConnectivity);

	virtual ~TaskSegmentation();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);

	bool reusable(ReusableTask* t);

	int serialize(char *buff);
	int deserialize(char *buff);
	ReusableTask* clone();
	int size();
};

#endif /* Segmentation_H_ */
