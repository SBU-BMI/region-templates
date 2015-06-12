/*
 * CompSeg.cpp
 *
 *  Created on: Feb 16, 2012
 *      Author: george
 */

#include "CompSeg.h"

CompSeg::CompSeg() {
	setComponentName("CompSeg");
}

CompSeg::~CompSeg() {
}

int CompSeg::run()
{
	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;

	std::string inputFileName = ((ArgumentString*)this->getArgument(0))->getArgValue();
	std::string outputMaskFileName = ((ArgumentString*)this->getArgument(1))->getArgValue();

	// create monalitic task that does all
	TaskDoAll* taskAll = new TaskDoAll(inputFileName);

	this->executeTask(taskAll);
	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactorySeg() {
	return new CompSeg();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister("CompSeg", &componentFactorySeg);


