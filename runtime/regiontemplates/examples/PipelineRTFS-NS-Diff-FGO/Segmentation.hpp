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

	std::list<ReusableTask*> mergable_t1;

public:

	Segmentation();
	virtual ~Segmentation();

	void set_workflow_id(int id) {workflow_id = id;};

	void print();

	int run();
};

// Task1Segmentation header
class TaskSegmentation0: public ReusableTask {
friend class TaskSegmentation1;
private:

	// all parameters
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	double T1;
	double T2;
	
	// intertask arguments
	std::shared_ptr<std::vector<cv::Mat>> bgr;
	std::shared_ptr<cv::Mat> rbc;

public:
	// data regions
	std::shared_ptr<DenseDataRegion2D*> normalized_rt_temp;

	TaskSegmentation0();
	TaskSegmentation0(list<ArgumentBase*> args, RegionTemplate* inputRt);

	virtual ~TaskSegmentation0();

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
class TaskSegmentation1: public ReusableTask {
friend class TaskSegmentation2;
private:

	// all parameters
	int reconConnectivity;
	
	// intertask arguments
	std::shared_ptr<std::vector<cv::Mat>> bgr;
	std::shared_ptr<cv::Mat> rc;
	std::shared_ptr<cv::Mat> rc_recon;
	std::shared_ptr<cv::Mat> rc_open;

	// forward intertask arguments
	std::shared_ptr<cv::Mat> rbc_fw;

public:
	// data regions

	TaskSegmentation1();
	TaskSegmentation1(list<ArgumentBase*> args, RegionTemplate* inputRt);

	virtual ~TaskSegmentation1();

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

// TaskSegmentation2 header
class TaskSegmentation2: public ReusableTask {
friend class TaskSegmentation3;
private:

	// all parameters
	int fillHolesConnectivity;
	unsigned char G1;
	
	// intertask arguments
	std::shared_ptr<cv::Mat> rc;
	std::shared_ptr<cv::Mat> rc_recon;
	std::shared_ptr<cv::Mat> rc_open;
	std::shared_ptr<cv::Mat> bw1;
	std::shared_ptr<cv::Mat> diffIm;

	// forward intertask arguments
	std::shared_ptr<cv::Mat> rbc_fw;

public:
	// data regions

	TaskSegmentation2();
	TaskSegmentation2(list<ArgumentBase*> args, RegionTemplate* inputRt);

	virtual ~TaskSegmentation2();

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

// TaskSegmentation3 header
class TaskSegmentation3: public ReusableTask {
friend class TaskSegmentation4;
private:

	// all parameters
	int minSize;
	int maxSize;
	
	// intertask arguments
	std::shared_ptr<cv::Mat> bw1;
	std::shared_ptr<cv::Mat> bw1_t;

	// forward intertask arguments
	std::shared_ptr<cv::Mat> rbc_fw;
	std::shared_ptr<cv::Mat> diffIm_fw;

public:
	// data regions

	TaskSegmentation3();
	TaskSegmentation3(list<ArgumentBase*> args, RegionTemplate* inputRt);

	virtual ~TaskSegmentation3();

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

// TaskSegmentation4 header
class TaskSegmentation4: public ReusableTask {
friend class TaskSegmentation5;
private:

	// all parameters
	unsigned char G2;
	
	// intertask arguments
	std::shared_ptr<cv::Mat> diffIm;
	std::shared_ptr<cv::Mat> bw1_t;
	std::shared_ptr<cv::Mat> rbc;
	std::shared_ptr<cv::Mat> seg_open;

public:
	// data regions

	TaskSegmentation4();
	TaskSegmentation4(list<ArgumentBase*> args, RegionTemplate* inputRt);

	virtual ~TaskSegmentation4();

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

// Task5Segmentation5 header
class TaskSegmentation5: public ReusableTask {
friend class TaskSegmentation6;
private:

	// all parameters
	int minSizePl;
	int watershedConnectivity;
	
	// intertask arguments
	std::shared_ptr<cv::Mat> seg_open;
	std::shared_ptr<cv::Mat> seg_nonoverlap;

public:
	// data regions
	std::shared_ptr<DenseDataRegion2D*> normalized_rt_temp;

	TaskSegmentation5();
	TaskSegmentation5(list<ArgumentBase*> args, RegionTemplate* inputRt);

	virtual ~TaskSegmentation5();

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

// TaskSegmentation6 header
class TaskSegmentation6: public ReusableTask {
private:

	// all parameters
	int minSizeSeg;
	int maxSizeSeg;
	int fillHolesConnectivity;
	
	// intertask arguments
	std::shared_ptr<cv::Mat> seg_open;
	std::shared_ptr<cv::Mat> seg_nonoverlap;

public:
	// data regions
	std::shared_ptr<DenseDataRegion2D*> segmented_rt_temp;

	TaskSegmentation6();
	TaskSegmentation6(list<ArgumentBase*> args, RegionTemplate* inputRt);

	virtual ~TaskSegmentation6();

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
