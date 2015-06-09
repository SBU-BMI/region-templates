
#ifndef TASKTILERECON_H_
#define TASKTILERECON_H_

#include "Task.h"
#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "ObjFeatures.h"

class TaskTileRecon:public Task {
private:
	Mat *markerImage;
	Mat *maskImage;

	int tileX, tileY,  tileSize;

public:
	TaskTileRecon(::cv::Mat* markerImage, ::cv::Mat* maskImage, int tileX, int TileY, int tileSize);
	~TaskTileRecon();

	// Perform computation of the nuclei candidates
	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASKTILERECON_H_ */
