/*
 * FireTrackingComponent.cpp
 *
 *  Created on: Feb 7, 2014
 *      Author: george
 */

#include "FireTrackingComponent.h"


FireTrackingComponent::FireTrackingComponent() {
	this->setComponentName("FireTrackingComponent");
	this->addInputOutputDataRegion("RTInput", "drInput", RTPipelineComponentBase::INPUT);
}

FireTrackingComponent::~FireTrackingComponent() {

}

int FireTrackingComponent::run()
{
	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;

	// Create tracking computing task
	TaskTracking* tt = new TaskTracking(this->getId());
	this->executeTask(tt);

	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactorySeg() {
	return new FireTrackingComponent();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister("FireTrackingComponent", &componentFactorySeg);


