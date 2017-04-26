/*
 * HelloWorld.cpp
 *
 *  Created on: Feb 15, 2012
 *      Author: george
 */
#include "SysEnv.h"
#include "Segmentation.h"
#include "FeatureExtraction.h"

#include <sstream>
#include <stdlib.h>
#include <iostream>

#include "FileUtils.h"
#include "RegionTemplate.h"
#include "RegionTemplateCollection.h"


#define NUM_PIPELINE_INSTANCES	1


void parseInputArguments(int argc, char**argv, std::string &inputFolder){
	// Used for parameters parsing
	for(int i = 0; i < argc-1; i++){
		if(argv[i][0] == '-' && argv[i][1] == 'i'){
			inputFolder = argv[i+1];
		}
	}
}


RegionTemplateCollection* RTFromFiles(std::string inputFolderPath){

	// Search for input files in folder path
	FileUtils fileUtils("tif");
	std::vector<std::string> fileList;
	fileUtils.traverseDirectoryRecursive(inputFolderPath, fileList);
	RegionTemplateCollection* rtCollection = new RegionTemplateCollection();
	rtCollection->setName("inputimage");

	std::cout << "Input Folder: "<< inputFolderPath <<std::endl;

	// Create one region template instance for each input data file
	// (creates representations without instantiating them)
	for(int i = 0; i < fileList.size(); i++){
		DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
		ddr2d->setName("BGR");
		std::ostringstream oss;
		//oss << i;
        
        // extract filename from full path
        std:string fn(fileList[i]);
		size_t loc = fn.rfind('/');
		fn.erase(0, loc+1);
     
        // remove .tif extension
        fn.erase(fn.find_last_of("."), std::string::npos);

		oss << fn;
		ddr2d->setId(oss.str());
		ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
		ddr2d->setIsAppInput(true);
		ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
		ddr2d->setInputFileName(fileList[i]);
		RegionTemplate *rt = new RegionTemplate();
		rt->setName("tile");
		rt->insertDataRegion(ddr2d);
		rtCollection->addRT(rt);
	}

	return rtCollection;
}



int main (int argc, char **argv){
	// Folder when input data images are stored
	std::string inputFolderPath;
	std::vector<RegionTemplate *> inputRegionTemplates;
	RegionTemplateCollection *rtCollection;

	parseInputArguments(argc, argv, inputFolderPath);


    // Parameters
    float otsuRatio = 1.0;
    double curvatureWeight = 0.8;
    float sizeThld = 3;
    float sizeUpperThld = 200;
    double mpp = 0.25;
    float msKernel = 20.0;
    int levelsetNumberOfIteration = 100;
	

	// Handler to the distributed execution system environment
	SysEnv sysEnv;

	// Tell the system which libraries should be used
	sysEnv.startupSystem(argc, argv, "libcomponentsrtfsyi.so");

	// Create region templates description without instantiating data
	rtCollection = RTFromFiles(inputFolderPath);

	// Build application dependency graph

	// Instantiate application dependency graph
	for(int i = 0; i < rtCollection->getNumRTs(); i++){
		Segmentation *seg = new Segmentation();
		seg->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
        seg->addArgument(new ArgumentFloat(otsuRatio));
        seg->addArgument(new ArgumentFloat(curvatureWeight));
        seg->addArgument(new ArgumentFloat(sizeThld));
        seg->addArgument(new ArgumentFloat(sizeUpperThld));
        seg->addArgument(new ArgumentFloat(mpp));
        seg->addArgument(new ArgumentFloat(msKernel));
        seg->addArgument(new ArgumentInt(levelsetNumberOfIteration));

		FeatureExtraction *fe =  new FeatureExtraction();

        fe->addArgument(new ArgumentFloat(otsuRatio));
        fe->addArgument(new ArgumentFloat(curvatureWeight));
        fe->addArgument(new ArgumentFloat(sizeThld));
        fe->addArgument(new ArgumentFloat(sizeUpperThld));
        fe->addArgument(new ArgumentFloat(mpp));
        fe->addArgument(new ArgumentFloat(msKernel));
        fe->addArgument(new ArgumentInt(levelsetNumberOfIteration));
		fe->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
		fe->addDependency(seg->getId());

		sysEnv.executeComponent(seg);
		sysEnv.executeComponent(fe);
	}

	// End Create Dependency Graph
	sysEnv.startupExecution();

	// Finalize all processes running and end execution
	sysEnv.finalizeSystem();

	delete rtCollection;

	return 0;
}



