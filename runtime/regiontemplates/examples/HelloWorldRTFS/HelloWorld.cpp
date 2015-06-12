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
		oss << i;
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

	// Handler to the distributed execution system environment
	SysEnv sysEnv;

	// Tell the system which libraries should be used
	sysEnv.startupSystem(argc, argv, "libcomponentsrtfs.so");

	// Create region templates description without instantiating data
	rtCollection = RTFromFiles(inputFolderPath);

	// Build application dependency graph

	// Instantiate application dependency graph
	for(int i = 0; i < rtCollection->getNumRTs(); i++){
		Segmentation *seg = new Segmentation();
		seg->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
		FeatureExtraction *fe =  new FeatureExtraction();

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



