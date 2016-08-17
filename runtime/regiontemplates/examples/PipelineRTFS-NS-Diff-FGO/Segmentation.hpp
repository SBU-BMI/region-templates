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
#include "ReusableTask.hpp"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"

// PipelineComponent header
class Segmentation : public RTPipelineComponentBase {

private:
	// data region id
	// IMPORTANT: this need to be set during the creation of this object
	int workflow_id;

	list<ReusableTask*> mergable_t1;

public:
	Segmentation();
	virtual ~Segmentation();

	void set_workflow_id(int id) {workflow_id = id;};

	void print();

	int run();
};

// Task1Segmentation header
class Task1Segmentation: public ReusableTask {
friend class Task2Segmentation;
private:

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
	int fillHolesConnectivity;
	int reconConnectivity;
	
	// intertask arguments
	cv::Mat *seg_open;

public:

	// data regions
	DenseDataRegion2D* normalized_rt_temp;

	Task1Segmentation() {};
	Task1Segmentation(list<ArgumentBase*> args, RegionTemplate* inputRt);

	virtual ~Task1Segmentation();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);

	bool reusable(ReusableTask* t);
	void updateDR(RegionTemplate* rt);
	void updateInterStageArgs(ReusableTask* t);
	void resolveDependencies(ReusableTask* t);

	int serialize(char *buff);
	int deserialize(char *buff);
	ReusableTask* clone();
	int size();

	void print();
};

// Task2Segmentation header
class Task2Segmentation: public ReusableTask {
friend class Task3Segmentation;
private:

	// all other variables
	int minSizePl;
	int watershedConnectivity;

	// intertask arguments
	cv::Mat **seg_open;
	cv::Mat *seg_nonoverlap;

public:

	// data regions
	DenseDataRegion2D* normalized_rt_temp;

	Task2Segmentation() {};
	Task2Segmentation(list<ArgumentBase*> args, RegionTemplate* inputRt);

	virtual ~Task2Segmentation();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);

	bool reusable(ReusableTask* t);
	void updateDR(RegionTemplate* rt);
	void updateInterStageArgs(ReusableTask* t);
	void resolveDependencies(ReusableTask* t);

	int serialize(char *buff);
	int deserialize(char *buff);
	ReusableTask* clone();
	int size();

	void print();
};

// Task3Segmentation header
class Task3Segmentation: public ReusableTask {
private:

	// all other variables
	int minSizeSeg;
	int maxSizeSeg;
	int fillHolesConnectivity;

	// intertask arguments
	cv::Mat **seg_nonoverlap;

public:

	// data regions
	DenseDataRegion2D* segmented_rt_temp;

	Task3Segmentation() {};
	Task3Segmentation(list<ArgumentBase*> args, RegionTemplate* inputRt);

	virtual ~Task3Segmentation();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);

	bool reusable(ReusableTask* t);
	void updateDR(RegionTemplate* rt);
	void updateInterStageArgs(ReusableTask* t);
	void resolveDependencies(ReusableTask* t);

	int serialize(char *buff);
	int deserialize(char *buff);
	ReusableTask* clone();
	int size();

	void print();
};

#endif /* Segmentation_H_ */
