

#include "DiffMaskComp.h"
#include "TaskDiffMask.h"

DiffMaskComp::DiffMaskComp() {
	this->setComponentName("DiffMaskComp");
	this->addInputOutputDataRegion("tile", "MASK", RTPipelineComponentBase::INPUT);
	this->addInputOutputDataRegion("tile", "FEATURES", RTPipelineComponentBase::INPUT);

}

DiffMaskComp::~DiffMaskComp() {

}

int DiffMaskComp::run()
{
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	if(inputRt != NULL){
		DenseDataRegion2D *mask0 = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("MASK", 0, 0));
		DenseDataRegion2D *mask1 = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("MASK", 0, 1));
		DenseDataRegion2D *fea0 = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("FEATURES", 0, 0));
		DenseDataRegion2D *fea1 = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("FEATURES", 0, 1));
		if(mask0 != NULL && mask1 != NULL && fea0 != NULL && fea1 != NULL){

			std::cout << "DiffMaskComp:: all data regions are not null."<< std::endl;
			// Create processing task
			TaskDiffMask *tDiffMask = new TaskDiffMask(mask0, mask1, fea0, fea1);

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


