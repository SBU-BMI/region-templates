/*
 * CallBackComponentExecution.cpp
 *
 *  Created on: Feb 22, 2012
 *      Author: george
 */

#include "CallBackComponentExecution.h"

CallBackComponentExecution::CallBackComponentExecution(PipelineComponentBase* compInst, Worker* worker) {
	this->setComponentId(compInst->getId());
	this->setWorker(worker);
	this->setCompInst(compInst);
}

int CallBackComponentExecution::getComponentId() const
{
    return componentId;
}

Worker *CallBackComponentExecution::getWorker() const
{
    return worker;
}

void CallBackComponentExecution::setWorker(Worker *worker)
{
    this->worker = worker;
}

void CallBackComponentExecution::setComponentId(int componentId)
{
    this->componentId = componentId;
}

CallBackComponentExecution::~CallBackComponentExecution() {
	delete compInst;
}

bool CallBackComponentExecution::run(int procType, int tid)
{
	Worker *curWorker = this->getWorker();
#ifdef DEBUG
	std::cout << "Running callback. Component id: "<< this->getCompInst()->getId() << std::endl;
#endif
	this->getWorker()->deleteActiveComponentRef(this->getCompInst());

	// if this is a RT component it means that may be a set of region templates instances
	// associated to this components, and any new data regions need to be reported to the manager
	if(compInst->getType() == PipelineComponentBase::RT_COMPONENT_BASE){

#ifdef WITH_RT			
		// this is a region template pipeline component
//		std::cout << std::endl<< "This is a region template component base" << std::endl << std::endl;
		PipelineComponentBase *rtComp = this->getCompInst();
		// serialize all the region templates within this component.

		// Stage region templates
		RTPipelineComponentBase *rtCompCast = dynamic_cast<RTPipelineComponentBase*>(rtComp);

		// Write all the output region template data regions to the staging area
		rtCompCast->stageRegionTemplates();

		// size of the data space need to store this component
		int size_bytes = rtComp->size();

		// alloc memory to store component data
		char *data = new char[size_bytes];

		// store component information into buffer
		rtComp->serialize(data);

		// Add id of the associated component to the list of components computed
		curWorker->addComputedComponent(this->getComponentId(), size_bytes, data);
#else
		std::cout << "System compiled without support for Region Templates, but application component is RT_COMPONENT_BASE TYPE. Please, recompile system. existing..." << std::endl;
		exit(1);
#endif		

	}else{
//		std::cout << "ERROR: Component not RT type: "<< compInst->getType()<< std::endl;
//		exit(1);
		// Add id of the associated component to the list of components computed
		curWorker->addComputedComponent(this->getComponentId());
	}
	std::cout << "Worker: CallBack: Component id="<< this->getComponentId() << " finished!" <<std::endl;

	return true;
}



