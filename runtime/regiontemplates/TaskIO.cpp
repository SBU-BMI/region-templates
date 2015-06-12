/*
 * TaskIO.cpp
 *
 *  Created on: Feb 21, 2013
 *      Author: george
 */

#include "TaskIO.h"

TaskIO::TaskIO() {
	this->associatedComponent = NULL;
	this->setTaskType(ExecEngineConstants::IO_TASK);
}

TaskIO::~TaskIO() {

}

TaskIO::TaskIO(RTPipelineComponentBase* associatedComponent) {
	this->associatedComponent = associatedComponent;
	this->setTaskType(ExecEngineConstants::IO_TASK);
}

bool TaskIO::run(int procType, int tid) {
#ifdef DEBUG
	std::cout << "Executing RT IO TASK" << std::endl;
#endif
	this->associatedComponent->instantiateRegionTemplates();

	return true;
}
