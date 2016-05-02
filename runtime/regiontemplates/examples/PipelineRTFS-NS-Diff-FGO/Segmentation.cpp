/*
 * Segmentation.cpp
 *
 *  Created on: Feb 15, 2013
 *      Author: george
 */

#include "Segmentation.h"


Segmentation::Segmentation() {
	this->setComponentName("Segmentation");
	this->addInputOutputDataRegion("tile", "BGR", RTPipelineComponentBase::INPUT);//_OUTPUT
	this->addInputOutputDataRegion("tile", "MASK", RTPipelineComponentBase::OUTPUT);//_OUTPUT
}

Segmentation::~Segmentation() {

}

int Segmentation::run()
{

	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	int parameterId = ((ArgumentInt*)this->getArgument(0))->getArgValue();
	int parameterSegId = ((ArgumentInt*)this->getArgument(1))->getArgValue();
	unsigned char blue = (unsigned char)((ArgumentInt*)this->getArgument(2))->getArgValue();
	unsigned char green = (unsigned char)((ArgumentInt*)this->getArgument(3))->getArgValue();
	unsigned char red = (unsigned char)((ArgumentInt*)this->getArgument(4))->getArgValue();
	double T1 = (double)((ArgumentFloat*)this->getArgument(5))->getArgValue();
	double T2 = (double)((ArgumentFloat*)this->getArgument(6))->getArgValue();
	unsigned char G1 = (unsigned char)((ArgumentInt*)this->getArgument(7))->getArgValue();
	unsigned char G2 = (unsigned char)((ArgumentInt*)this->getArgument(8))->getArgValue();
	int minSize = ((ArgumentInt*)this->getArgument(9))->getArgValue();
	int maxSize = ((ArgumentInt*)this->getArgument(10))->getArgValue();
	int minSizePl = ((ArgumentInt*)this->getArgument(11))->getArgValue();
	int minSizeSeg = ((ArgumentInt*)this->getArgument(12))->getArgValue();
	int maxSizeSeg= ((ArgumentInt*)this->getArgument(13))->getArgValue();
	int fillHolesConnectivity= ((ArgumentInt*)this->getArgument(14))->getArgValue();
	int reconConnectivity= ((ArgumentInt*)this->getArgument(15))->getArgValue();
	int watershedConnectivity= ((ArgumentInt*)this->getArgument(16))->getArgValue();

	if(inputRt != NULL){
		DenseDataRegion2D *bgr = NULL;
		try{
			bgr = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("BGR", "", 0, parameterId));
			std::cout << "Segmentation. paramenterId: "<< parameterId<<std::endl;
		}catch(...){
			std::cout <<"ERROR SEGMENTATION " << std::endl;
			bgr=NULL;
		}
		if(bgr != NULL){
			std::cout << "Segmentation. BGR input id: "<< bgr->getId() << " paramenterId: "<< parameterId<<std::endl;
			// Create output data region
			DenseDataRegion2D *mask = new DenseDataRegion2D();
			mask->setName("MASK");
			mask->setId(bgr->getId());
			mask->setVersion(parameterSegId);

			inputRt->insertDataRegion(mask);
			std::cout <<  "nDataRegions: after:" << inputRt->getNumDataRegions() << std::endl;

			// Create processing tasks
			TaskSegmentationStg1 * segTask1 = new TaskSegmentationStg1(bgr, blue, 
				green, red, T1, T2, G1, minSize, maxSize, G2, fillHolesConnectivity, 
				reconConnectivity, &findCandidateResult, &seg_norbc, NULL, NULL);

			TaskSegmentationStg2 * segTask2 = new TaskSegmentationStg2(&findCandidateResult, 
				&seg_norbc, &seg_nohole,NULL, NULL);
			TaskSegmentationStg3 * segTask3 = new TaskSegmentationStg3(&seg_nohole, 
				&seg_open, NULL, NULL);
			img = bgr->getData();
			TaskSegmentationStg4 * segTask4 = new TaskSegmentationStg4(&img, minSizePl, 
				watershedConnectivity, &seg_open, &seg_nonoverlap, NULL, NULL);
			TaskSegmentationStg5 * segTask5 = new TaskSegmentationStg5(minSizeSeg, 
				maxSizeSeg, &seg_nonoverlap, &seg, NULL, NULL);
			TaskSegmentationStg6 * segTask6 = new TaskSegmentationStg6(&seg, mask, fillHolesConnectivity, NULL, NULL);

			// set dependencies
			segTask2->addDependency(segTask1);
			segTask3->addDependency(segTask2);
			segTask4->addDependency(segTask3);
			segTask5->addDependency(segTask4);
			segTask6->addDependency(segTask5);
			
			this->executeTask(segTask1);
			this->executeTask(segTask2);
			this->executeTask(segTask3);
			this->executeTask(segTask4);
			this->executeTask(segTask5);
			this->executeTask(segTask6);

		}else{
			std::cout << __FILE__ << ":" << __LINE__ <<" DR == NULL" << std::endl;
		}

	}else{
		std::cout << __FILE__ << ":" << __LINE__ <<" RT == NULL" << std::endl;
	}

	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactorySeg() {
	return new Segmentation();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister("Segmentation", &componentFactorySeg);


