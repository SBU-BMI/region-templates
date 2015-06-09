
#include "PipelineComponent.h"
#include "TaskCompute.h"
#include "TaskRead.h"

PipelineComponent::PipelineComponent() {
	this->setComponentName("PipelineComponent");
}

PipelineComponent::~PipelineComponent() {

}

int PipelineComponent::run()
{
	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;

	// Retrieve an argument value, and transform it to integer
	std::string inputFileName = ((ArgumentString*)this->getArgument(0))->getArgValue();

	// Create image reading task
	TaskRead* tRead = new TaskRead(inputFileName);
	tRead->setSpeedup(ExecEngineConstants::CPU, 1.0);
	tRead->setSpeedup(ExecEngineConstants::GPU, 1.0);
	
	// Create internal pipeline
	// Fist computing task 
	TaskCompute* tRBC = new TaskCompute(394100, 29910, 30000, "RBC");
	tRBC->addDependency(tRead);
	
	// Second task depending on tRBC
	TaskCompute* tMorphOpen = new TaskCompute(152231, 16536, 30000, "MorphOpen");
	tMorphOpen->addDependency(tRBC);

	// 3rd task depending on tMorphOpen
	TaskCompute* tReconNuclei = new TaskCompute(1748298, 318883, 30000, "ReconNuclei");
	tReconNuclei->addDependency(tMorphOpen);
	
	// 4th task depending on tReconNuclei
	TaskCompute* tAreaThreshold = new TaskCompute(129505, 28102, 30000, "AreaThreshold");
	tAreaThreshold->addDependency(tReconNuclei);

	// 5th task depending on tAreaThreshold
	TaskCompute* tDistTransform = new TaskCompute(1587335, 229910, 30000, "DistTransform");
	tDistTransform->addDependency(tAreaThreshold);
	
	// 6th task depending on tDistTransform
	TaskCompute* tCCL = new TaskCompute(355500, 62000, 30000, "CCL");
	tCCL->addDependency(tDistTransform);
	
	// 7th task depending w/ no dependency
	TaskCompute* tColorDeconv = new TaskCompute(1096914, 78151, 30000, "ColorDeconv");
	
	// 8th task depending w/ no dependency
	TaskCompute* tPixelStats = new TaskCompute(151712, 8973, 30000, "PixelStats");
	tPixelStats->addDependency(tCCL);

	// 9th task depending w/ no dependency
	TaskCompute* tGradientStats = new TaskCompute(1723064, 96830, 30000, "GradientStats");
	tGradientStats->addDependency(tCCL);
	tGradientStats->addDependency(tColorDeconv);

	// 10th task depending w/ no dependency
	TaskCompute* tSobelStats = new TaskCompute(294220, 17888, 30000, "SobelStats");
	tSobelStats->addDependency(tCCL);
	tSobelStats->addDependency(tColorDeconv);
	
	// Dispatch tasks for execution with Resource Manager.
	this->executeTask(tRead);
	this->executeTask(tRBC);
	this->executeTask(tMorphOpen);
	this->executeTask(tReconNuclei);
	this->executeTask(tAreaThreshold);
	this->executeTask(tDistTransform);
	this->executeTask(tCCL);
	this->executeTask(tColorDeconv);
	this->executeTask(tPixelStats);
	this->executeTask(tGradientStats);
	this->executeTask(tSobelStats);

	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactoryA() {
	return new PipelineComponent();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister("PipelineComponent", &componentFactoryA);


