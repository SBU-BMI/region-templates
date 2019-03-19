/*
 * Segmentation.h
 *
 *  Created on: Feb 12, 2013
 *  Author: george
 */

#ifndef SEGMENTATION_H_
#define SEGMENTATION_H_

#include "RTPipelineComponentBase.h"

#include "opencv2/opencv.hpp"
// #include "opencv2/gpu/gpu.hpp" // old opencv 2.4
#include "opencv2/cudaarithm.hpp" // new opencv 3.4.1

#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "FileUtils.h"


#include "TaskSegmentation.h"

class Segmentation : public RTPipelineComponentBase {
private:
	cv::Mat inputImage;
	cv::Mat outLabeledMask;

public:
	Segmentation();
	virtual ~Segmentation();

	int run();

};

#endif /* SEGMENTATION_H_ */
