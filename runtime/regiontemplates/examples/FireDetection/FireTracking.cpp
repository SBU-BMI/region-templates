/*
 * FireDetection.cpp
 *
 *  Created on: Feb 07, 2012
 *      Author: george
 */

#include <sstream>
#include <stdlib.h>
#include <iostream>

#include "FileUtils.h"
#include "RegionTemplate.h"
#include "RegionTemplateCollection.h"

#include "SysEnv.h"
#include  "FireTrackingComponent.h"
//#include "Segmentation.h"
//#include "FeatureExtraction.h"

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
	FileUtils fileUtils(".png");
	std::vector<std::string> fileList;
	fileUtils.traverseDirectoryRecursive(inputFolderPath, fileList);
	RegionTemplateCollection* rtCollection = new RegionTemplateCollection();
	rtCollection->setName("RTInput");

	std::cout << "Input Folder: "<< inputFolderPath <<std::endl;

	RegionTemplate *rt = new RegionTemplate();
	rt->setName("RTInput");

	// Create one data region  instance for each input data file
	// (creates representations without instantiating them)
	for(int i = 0; i < fileList.size(); i++){
		DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
		ddr2d->setName("drInput");
		ddr2d->setInputType(DataSourceType::FILE_SYSTEM);

		// BEGIN: get information related to the data region location
		// Get file without path
		unsigned found = fileList[i].find_last_of("/\\");
		string fname = fileList[i].substr(found+1);

		size_t pos = 0;
		string delimiter = "-";
		std::string token;
		std::vector<int> fileLocation;
		while ((pos = fname.find(delimiter)) != std::string::npos) {
		    token = fname.substr(0, pos);
//		    std::cout << token << std::endl;
		    fname.erase(0, pos + delimiter.length());
		    fileLocation.push_back(atoi(token.c_str()));
		}
		int timeStamp = atoi(fname.erase(fname.length(), fname.length()-4).c_str());
//		std::cout << "TimeStamp: "<< timeStamp << std::endl;
		// END: parsing done in the file name

		ddr2d->setTimestamp(timeStamp);
		ddr2d->setROI(BoundingBox(Point(0,0,0), Point(319,239,0)));
		ddr2d->insertBB2IdElement(ddr2d->getROI(),fileList[i]);

		ddr2d->setId(fileList[i]);
		std::cout << "Before insertDataRegion. Fname: "<< fileList[i] << std::endl;
		rt->insertDataRegion(ddr2d);

	}
	rt->setLazyRead(false);
	rtCollection->addRT(rt);

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

	// Tell the system which libraries implement the components should be used
	sysEnv.startupSystem(argc, argv, "libcomponentsrt.so");

	// Create region templates description without instantiating data
	rtCollection = RTFromFiles(inputFolderPath);

	// Instantiate application dependency graph
	for(int i = 0; i < rtCollection->getNumRTs(); i++){
		FireTrackingComponent *ftc = new FireTrackingComponent();
		ftc->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
		sysEnv.executeComponent(ftc);
	}

	// End Create Dependency Graph
	sysEnv.startupExecution();

	// Finalize all processes running and end execution
	sysEnv.finalizeSystem();

	delete rtCollection;

	return 0;
}



