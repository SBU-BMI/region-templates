
#ifndef TASKFIXRECON_H_
#define TASKFIXRECON_H_

#include "Task.h"
#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "ObjFeatures.h"

class TaskFixRecon:public Task {
private:
	Mat *markerImage;
	Mat *maskImage;
	Mat *originalMarkerImage;

	int tileSize;

public:
	TaskFixRecon(::cv::Mat* markerImage, ::cv::Mat* originalMarkerImage, ::cv::Mat* maskImage, int tileSize);
	~TaskFixRecon();


	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASKFIXRECON_H_ */
