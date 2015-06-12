
#ifndef TASKFIXPROP_H_
#define TASKFIXPROP_H_

#include "Task.h"
#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "ObjFeatures.h"

class TaskFixProp:public Task {
private:
	Mat *neighbor;
	Mat *originalMaskImage;

	int tileSize;

public:
	TaskFixProp(::cv::Mat* maskImage, ::cv::Mat* originalMaskImage, int tileSize);
	~TaskFixProp();


	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASKFIXPROP_H_ */
