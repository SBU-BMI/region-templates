/*
 * CompPrint.cpp
 *
 *  Created on: Feb 16, 2012
 *      Author: george
 */

#include "CompPrint.h"
#include "TaskSum.h"

CompPrint::CompPrint() {
	this->setComponentName("CompPrint");
}

CompPrint::~CompPrint() {

}

int CompPrint::run()
{
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;

	// Allocate input/output buffers
	int *b = new int[1];
	b[0] = atoi( ((std::string *)this->getArgument(0)->getArgPtr())->c_str());
	int *c = new int[1]; c[0] = 2;
	int *k1 = new int[1]; k1[0] = 10;
	int *k2 = new int[1]; k2[0] = 5;
	int *aOut = new int[1];
	int *bOut = new int[1];
	int *cOut = new int[1];
	// End memory allocation section

	// Part that matters

	TaskSum* tA = new TaskSum(aOut, b, c, "A");

	TaskSum* tB = new TaskSum(bOut, aOut, k1, "B");
	tB->addDependency(tA);

	TaskSum* tC = new TaskSum(cOut, bOut, k2, "C");
	tC->addDependency(tB);

	// Insert tasks
	this->executeTask(tA);
	this->executeTask(tB);
	this->executeTask(tC);
}

// Create the component factory
componentFactoryFunction(CompPrint);

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister("CompPrint", &componentFactory);


