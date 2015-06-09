
#ifndef COMPSEG_H_
#define COMPSEG_H_


#include "Task.h"
#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PipelineComponentBase.h"

// The single substages of this pipeline component
#include "TaskDoAll.h"

class CompSeg : public PipelineComponentBase {

public:
	CompSeg();
	virtual ~CompSeg();

	int run();

};

#endif
