

#ifndef PIPELINECOMPONENT_H_
#define PIPELINECOMPONENT_H

#include "PipelineComponentBase.h"

class PipelineComponent : public PipelineComponentBase {
private:

public:
	PipelineComponent();
	virtual ~PipelineComponent();

	int run();

};

#endif
