

#ifndef TASK_SEGMENTATION_H_
#define TASK_SEGMENTATION_H_

#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
//#include "HistologicalEntities.h"
//#include "PixelOperations.h"
//#include "MorphologicOperations.h"
#include "Util.h"
//#include "FileUtils.h"
//#include "ProcessTileUtils.h"

//itk
//#include "itkImage.h"
//#include "itkRGBPixel.h"
//#include "itkImageFileWriter.h"
//#include "itkOtsuThresholdImageFilter.h"
//#include "itkCastImageFilter.h"
//#include "itkOpenCVImageBridge.h"
//
//
//#include "utilityTileAnalysis.h"

class TaskSegmentation: public Task {
private:
	DenseDataRegion2D* bgr;
	DenseDataRegion2D* mask;

	float otsuRatio, curvatureWeight, sizeThld, sizeUpperThld, mpp, mskernel;
	int levelSetNumberOfIteration;


public:
	TaskSegmentation(DenseDataRegion2D *bgr, DenseDataRegion2D *mask, float otsuRatio, float curvatureWeight,
					 float sizeThld, float sizeUpperThld, float mpp, float mskernel, int levelSetNumberOfIteration);

	virtual ~TaskSegmentation();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_SEGMENTATION_H_ */
