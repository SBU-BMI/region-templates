/*
 * GeneralPipelineComponent.hpp
 *
 *  Created on: May 23, 2016
 *  Author: willian
 */

#ifndef GENETAL_TASK_H_
#define GENETAL_TASK_H_

#include "RTPipelineComponentBase.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "FileUtils.h"

class GeneralPipelineComponent : public RTPipelineComponentBase {

public:
	GeneralPipelineComponent(std::string name);
	virtual ~GeneralPipelineComponent();

	void GeneralPipelineComponent::addRtInput(std::string rtName, std::string regionName);
	void GeneralPipelineComponent::addRtOutput(std::string rtName, std::string regionName);

	int run();
};

#endif /* GENETAL_TASK_H_ */
