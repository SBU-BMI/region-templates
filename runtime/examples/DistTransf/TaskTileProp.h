
#ifndef TASKTILEPROP_H_
#define TASKTILEPROP_H_

#include "Task.h"
#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "ObjFeatures.h"

class TaskTileProp:public Task {
private:
	Mat *neighbor;
	Mat *maskImage;

	int tileX, tileY, tileSize, imgCols;

public:
	TaskTileProp(::cv::Mat* neighbor, ::cv::Mat* maskImage, int tileX, int TileY, int tileSize, int imgCols);
	~TaskTileProp( );

	// Perform computation of the nuclei candidates
	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASKTILEPROP_H_ */
