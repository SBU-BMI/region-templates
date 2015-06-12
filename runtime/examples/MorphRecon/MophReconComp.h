
#ifndef MORPHRECONCOMP_H_
#define MORPHRECONCOMP_H_


#include "Task.h"
#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PipelineComponentBase.h"
#include "ReadInputFileNames.h"

// The  substages of this pipeline component
#include "TaskTileRecon.h"
#include "TaskFixRecon.h"
#include "utils.h"

class MophReconComp : public PipelineComponentBase {
private:
	Mat *markerImage;
	Mat *maskImage;

	uint64_t t1, t2;

public:
	MophReconComp();
	virtual ~MophReconComp();

	int run();

};

#endif
