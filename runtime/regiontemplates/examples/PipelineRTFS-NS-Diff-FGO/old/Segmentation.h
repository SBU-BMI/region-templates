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
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "FileUtils.h"

#include "TaskSegmentationStg1.hpp"
#include "TaskSegmentationStg2.hpp"
#include "TaskSegmentationStg3.hpp"
#include "TaskSegmentationStg4.hpp"
#include "TaskSegmentationStg5.hpp"
#include "TaskSegmentationStg6.hpp"

class Segmentation : public RTPipelineComponentBase {
private:
	cv::Mat inputImage;
	cv::Mat outLabeledMask;

	// stg1-2 intertask variables
	int findCandidateResult;
	Mat seg_norbc;

	// stg2-3 intertask variables
	Mat seg_nohole;

	// stg3-4 intertask variables
	Mat seg_open;

	// stg4-5 intertask variables
	Mat img;
	Mat seg_nonoverlap;

	// stg5-6 intertask variables
	Mat seg;

public:
	Segmentation();
	virtual ~Segmentation();

	int run();

};

#endif /* SEGMENTATION_H_ */
