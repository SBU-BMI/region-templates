/*
 * CompA.cpp
 *
 *  Created on: Feb 16, 2012
 *      Author: george
 */

#include "CompA.h"
#include "TaskSum.h"

CompA::CompA() {
	this->setComponentName("CompA");
	this->c = 2;
	this->k1 = 10;
	this->k2 = 5;
}

CompA::~CompA() {

}

int CompA::run()
{
	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;

	// Retrieve an argument value, and transform it to integer
	b = atoi (((ArgumentString*)this->getArgument(0))->getArgValue().c_str());

	// Create internal pipeline
	// Fist tasks without dependencies
	TaskSum* tA = new TaskSum(&aOut, &b, &c, "A.1");

	// Second task depending on tA
	TaskSum* tB = new TaskSum(&bOut, &aOut, &k1, "A.2");
	tB->addDependency(tA);

	// Third depends on tB
	TaskSum* tC = new TaskSum(&cOut, &bOut, &k2, "A.3");
	tC->addDependency(tB);

	// Dispatch tasks for execution with Resource Manager.
	this->executeTask(tA);
	this->executeTask(tB);
	this->executeTask(tC);

	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactoryA() {
	return new CompA();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister("CompA", &componentFactoryA);


