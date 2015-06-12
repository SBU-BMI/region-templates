/*
 * CompB.cpp
 *
 *  Created on: Feb 16, 2012
 *      Author: george
 */

#include "CompB.h"
#include "TaskSum.h"

CompB::CompB() {
	this->setComponentName("CompB");
	this->c = 2;
	this->k1 = 10;
	this->k2 = 5;
}

CompB::~CompB() {

}

int CompB::run()
{
	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;

	// Retrieve an argument value, and transform it to integer
	b = atoi (((ArgumentString*)this->getArgument(0))->getArgValue().c_str());

	// Create internal pipeline
	// Fist tasks without dependencies
	TaskSum* tA = new TaskSum(&aOut, &b, &c, "B.1");

	// Second task depending on tA
/*	TaskSum* tB = new TaskSum(&bOut, &aOut, &k1, "B.2");
	tB->addDependency(tA);

	// Third depends on tA as well
	TaskSum* tC = new TaskSum(&cOut, &aOut, &k2, "B.3");
	tC->addDependency(tA);

	// Third depends on tA as well
	TaskSum* tD = new TaskSum(&dOut, &bOut, &cOut, "B.4");
	tD->addDependency(tB);
	tD->addDependency(tC);*/

	// Dispatch tasks for execution with Resource Manager.
	this->executeTask(tA);
//	this->executeTask(tB);
//	this->executeTask(tC);
//	this->executeTask(tD);

	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactoryB() {
	return new CompB();
}

// register factory with the runtime system
bool registeredB = PipelineComponentBase::ComponentFactory::componentRegister("CompB", &componentFactoryB);


