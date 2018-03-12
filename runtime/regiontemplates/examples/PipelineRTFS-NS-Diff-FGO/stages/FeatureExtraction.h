/*
 * FeatureExtraction.h
 *
 *  Created on: Feb 13, 2013
 *      Author: george
 */

#ifndef FEATURE_EXTRACTION_H_
#define FEATURE_EXTRACTION_H_

#include "TaskFeatures.h"
#include "RTPipelineComponentBase.h"


class FeatureExtraction : public RTPipelineComponentBase {
private:


public:
	FeatureExtraction();
	virtual ~FeatureExtraction();

	int run();

};

#endif /* FEATURE_EXTRACTION_H_ */
