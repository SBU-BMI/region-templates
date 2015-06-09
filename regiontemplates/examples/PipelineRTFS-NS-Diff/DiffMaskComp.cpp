

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
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	if(inputRt != NULL){
		DenseDataRegion2D *mask1 = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("MASK", 0, 0));
		DenseDataRegion2D *mask2 = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("MASK", 0, 1));
		if(mask1 != NULL && mask2 != NULL){
			// Create processing task
			TaskDiffMask *tDiffMask = new TaskDiffMask(mask1, mask2);

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


