

#include "DiffMaskComp.h"
#include "TaskDiffMask.h"

DiffMaskComp::DiffMaskComp() {
	this->setComponentName("DiffMaskComp");
	this->addInputOutputDataRegion("tile", "MASK", RTPipelineComponentBase::INPUT);
}

DiffMaskComp::~DiffMaskComp() {

}

int DiffMaskComp::run()
{
	int parameterSegId = ((ArgumentInt*)this->getArgument(0))->getArgValue();

	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	int *diffPixels = (int*) malloc(4 * sizeof(int));
	diffPixels[0] = diffPixels[1] = diffPixels[2] = diffPixels[3] = 0;
	this->setResultData((char*)diffPixels, 4* sizeof(float));

	if(inputRt != NULL){

		// Mask computed in segmentation using specific application parameter set
		DenseDataRegion2D *computed_mask = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("MASK", "", 0, parameterSegId));

		// Mask used as a reference
		DenseDataRegion2D *reference_mask = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("REF_MASK"));

		if(computed_mask != NULL && reference_mask != NULL){
			// gambiarra
			diffPixels[0] =  this->getId();
			// Create processing task
			TaskDiffMask *tDiffMask = new TaskDiffMask(computed_mask, reference_mask, diffPixels);

			this->executeTask(tDiffMask);
		}else{
			std::cout << "DiffMaskComp: did not find data regions: " << std::endl;
			inputRt->print();
		}
	}else{
		std::cout << "\tTASK diff mask: Did not find RT named tile"<< std::endl;
	}

	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactoryDF() {
	return new DiffMaskComp();
}

// register factory with the runtime system
bool registeredDF = PipelineComponentBase::ComponentFactory::componentRegister("DiffMaskComp", &componentFactoryDF);


