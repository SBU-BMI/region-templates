
#ifndef DISTCOMP_H_
#define DISTCOMP_H_


#include "Task.h"
#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PipelineComponentBase.h"
#include "ReadInputFileNames.h"

#include "TaskArgumentMat.h"

// The  substages of this pipeline component
#include "TaskTileProp.h"
#include "TaskFixProp.h"
#include "utils.h"

class DistComp : public PipelineComponentBase {
private:
	Mat *maskImage;
	Mat *neighborImage;

	uint64_t t1, t2;

public:
	DistComp();
	virtual ~DistComp();

	int run();

};

#endif
