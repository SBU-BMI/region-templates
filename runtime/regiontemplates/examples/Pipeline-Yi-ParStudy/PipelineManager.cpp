
#include "SysEnv.h"
#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <string>
#include "FileUtils.h"
#include "RegionTemplate.h"
#include "RegionTemplateCollection.h"
#include "Segmentation.h"
#include "DiffMaskComp.h"
#include "ParameterSet.h"
#include <fstream>                                                      
#include <iomanip>                                                      
#include <map>                                                          
#include <stdlib.h>                                                     
#include <vector>  

namespace patch{
	template < typename T > std::string to_string(const T& n)
	{
		std::ostringstream stm;
		stm << n;
		return stm.str();
	}
}
void parseInputArguments(int argc, char**argv, std::string &inputFolder, std::string &parametersFile){
	// Used for parameters parsing
	for(int i = 0; i < argc-1; i++){
		if(argv[i][0] == '-' && argv[i][1] == 'i'){
			inputFolder = argv[i+1];
		}
		if(argv[i][0] == '-' && argv[i][1] == 'a'){
			parametersFile = argv[i+1];
		}
	}
}


RegionTemplateCollection* RTFromFiles(std::string inputFolderPath){
	// Search for input files in folder path
	FileUtils fileUtils(".mask.png");
	std::vector<std::string> fileList;
	fileUtils.traverseDirectoryRecursive(inputFolderPath, fileList);
	RegionTemplateCollection* rtCollection = new RegionTemplateCollection();
	rtCollection->setName("inputimage");

	std::cout << "Input Folder: "<< inputFolderPath <<std::endl;
	if(fileList.size() > 0)
		std::cout << "FILELIST[0]: "<< fileList[0] <<std::endl;

	std::string temp;
	// Create one region template instance for each input data file
	// (creates representations without instantiating them)
	for(int i = 0; i < fileList.size(); i++){

		// Create input mask data region
		DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
		ddr2d->setName("RAW");
		std::ostringstream oss;
		oss << i;
		ddr2d->setId(oss.str());
		ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
		ddr2d->setIsAppInput(true);
		ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);

		std::string inputFileName = fileUtils.replaceExt(fileList[i], ".mask.png", ".tif");
		ddr2d->setInputFileName(inputFileName);

		// Create reference mask data region
		DenseDataRegion2D *ddr2dRefMask = new DenseDataRegion2D();
		ddr2dRefMask->setName("REF_MASK");
		ddr2dRefMask->setId(oss.str());
		ddr2dRefMask->setInputType(DataSourceType::FILE_SYSTEM);
		ddr2dRefMask->setIsAppInput(true);
		ddr2dRefMask->setOutputType(DataSourceType::FILE_SYSTEM);
		ddr2dRefMask->setInputFileName(fileList[i]);

		// Adding data regions to region template
		RegionTemplate *rt = new RegionTemplate();
		rt->setName("tile");
		rt->insertDataRegion(ddr2d);
		rt->insertDataRegion(ddr2dRefMask);

		// Adding region template instance to collection
		rtCollection->addRT(rt);
	}

	return rtCollection;
}
void buildParameterSet(ParameterSet &segmentation, std::string parametersFile){                                         
	string line;
	map<std::string, vector<float> > parameters;                                                                            
	ifstream myfile (parametersFile.c_str());                                                                                                    
	if (myfile.is_open())                                                                                                                        
	{                                                                                                                                            
		while ( getline (myfile,line) )                                                                                                      
		{                                                                                                                                    
			std::string name;                                                                                                            
			vector<float> param(3);                                                                                                      
			std::istringstream iss(line);                                                                                                
			iss >> name;                                                                                                                 
			for(int i = 0; i < 3; i++)                                                                                                   
				iss >> param[i];                                                                                                     
			parameters.insert(make_pair(name, param));                                                                                   
		}                                                                                                                                    
		myfile.close();                                                                                                                      
	}else{                                                                                                                                       
		cout << "Unable to open file: " <<parametersFile << std::endl ;                                                                      
		exit(1);                                                                                                                             
	}                                                                                                                                            

	std::cout <<  "BEFORE ASSERT" << std::endl;                                                                                                  
	assert(parameters.find("otsuRatio") != parameters.end());
	assert(parameters.find("curvatureWeight") != parameters.end());
	assert(parameters.find("sizeThld") != parameters.end());
	assert(parameters.find("sizeUpperThld") != parameters.end());
	assert(parameters.find("msKernel") != parameters.end());
	assert(parameters.find("levelsetNumberOfIteration") != parameters.end());
	std::cout <<  "AFTER ASSERT" << std::endl;                                                                                                   

	// otsuRatio
	segmentation.addArgument(new ArgumentFloat(parameters["otsuRatio"][0]));

	// curvatureWeight
	segmentation.addArgument(new ArgumentFloat(parameters["curvatureWeight"][0]));
	
	// sizeThld
	segmentation.addArgument(new ArgumentFloat(parameters["sizeThld"][0]));
	
	// sizeUpperThld
	segmentation.addArgument(new ArgumentFloat(parameters["sizeUpperThld"][0]));

	// msKernel
	segmentation.addArgument(new ArgumentFloat(parameters["msKernel"][0]));
	
	// levelsetNumberOfIteration
	segmentation.addArgument(new ArgumentFloat(parameters["levelsetNumberOfIteration"][0]));


	return;
}

int main (int argc, char **argv){

	// Folder when input data images are stored
	std::string inputFolderPath, parametersFile;
	std::vector<RegionTemplate *> inputRegionTemplates;
	RegionTemplateCollection *rtCollection;
	std::vector<int> diffComponentIds;
	std::map<std::string, double> perfDataBase;


	parseInputArguments(argc, argv, inputFolderPath, parametersFile);

	// Handler to the distributed execution system environment
	SysEnv sysEnv;

	// Tell the system which libraries should be used
	sysEnv.startupSystem(argc, argv, "libcomponentsyiparstudy.so");

	// Create region templates description without instantiating data
	rtCollection = RTFromFiles(inputFolderPath);

	ParameterSet parSetSegmentation;
	buildParameterSet(parSetSegmentation, parametersFile);

	int segCount = 0;

	// Build application dependency graph
	// Instantiate application dependency graph
	for(int i = 0; i < rtCollection->getNumRTs(); i++){
		int previousSegCompId = 0;
		int versionSeg = 0;

		parSetSegmentation.resetIterator();

		// walk through parameter set and instantiate the computation pipeline for each parameter combination
		std::vector<ArgumentBase*> argSetInstanceSegmentation = parSetSegmentation.getNextArgumentSetInstance();

		Segmentation *seg = new Segmentation();

		// version of the data region generated by the segmentation stage
		seg->addArgument(new ArgumentInt(versionSeg));

		// add remaining (application specific) parameters from the argSegInstance
		for(int j = 0; j < 6; j++){
			seg->addArgument(argSetInstanceSegmentation[j]);
		}
		// and region template instance that it is suppose to process
		seg->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());

		std::cout<< "Creating DiffMask" << std::endl;
		DiffMaskComp *diff = new DiffMaskComp();

		// version of the data region that will be read. It is created during the segmentation.
		diff->addArgument(new ArgumentInt(versionSeg));

		// region template name
		diff->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
		diff->addDependency(seg->getId());

		// add to the list of diff component ids.
		diffComponentIds.push_back(diff->getId());

		sysEnv.executeComponent(seg);
		sysEnv.executeComponent(diff);

		std::cout << "Manager CompId: " << diff->getId() << " fileName: " << rtCollection->getRT(i)->getDataRegion(0)->getInputFileName() << std::endl;
		segCount++;
	}

	// End Creating Dependency Graph
	sysEnv.startupExecution();

	long long diffPixels = 0, foregroundPixels = 0; 
	for(int i = 0; i < diffComponentIds.size(); i++){
		char * resultData = sysEnv.getComponentResultData(diffComponentIds[i]);
		std::cout << "Diff Id: "<< diffComponentIds[i] << " resultData: ";
		if(resultData != NULL){
			std::cout << "size: " << ((int*)resultData)[0] << " diffPixels: "<< ((int*)resultData)[1]<< " refPixels: "<< ((int*)resultData)[2]<< std::endl;
			diffPixels+= ((int*)resultData)[1];
			foregroundPixels += ((int*)resultData)[2];
		}else{
			std::cout << "NULL" << std::endl;
		}
		sysEnv.eraseResultData(diffComponentIds[i]);
	}
	std::cout << "Total diff: "<< diffPixels << " total foreground: "<< foregroundPixels<< std::endl;      
	ofstream outfile ("results.out");                                                                      
	if (outfile.is_open())                                                                                 
		outfile << diffPixels<< std::endl;                                                             

	outfile.close();                         




	// Finalize all processes running and end execution
	sysEnv.finalizeSystem();

	delete rtCollection;

	return 0;
}



