
#include "SysEnv.h"
#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <string>


#include "FileUtils.h"
#include "RegionTemplate.h"
#include "RegionTemplateCollection.h"

#include "NormalizationComp.h"
#include "Segmentation.h"
#include "FeatureExtraction.h"
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
		std::string inputFileName = fileUtils.replaceExt(fileList[i], ".mask.png", ".tiff");
		ddr2d->setInputFileName(inputFileName);

		// Create reference mask data region
		DenseDataRegion2D *ddr2dRefMask = new DenseDataRegion2D();
		ddr2dRefMask->setName("REF_MASK");
		ddr2dRefMask->setId(oss.str());
		ddr2dRefMask->setInputType(DataSourceType::FILE_SYSTEM);
		ddr2dRefMask->setIsAppInput(true);
		ddr2dRefMask->setOutputType(DataSourceType::FILE_SYSTEM);
		ddr2dRefMask->setInputFileName(fileList[i]);

	/*	// Create reference mask data region
		DenseDataRegion2D *ddr2dRefNorm = new DenseDataRegion2D();
		ddr2dRefNorm->setName("NORM");
		ddr2dRefNorm->setId(oss.str());
		ddr2dRefNorm->setInputType(DataSourceType::FILE_SYSTEM);
		ddr2dRefNorm->setIsAppInput(true);
		ddr2dRefNorm->setOutputType(DataSourceType::FILE_SYSTEM);
		ddr2dRefNorm->setInputFileName("/home/george/workspace/nscale-normalization/build/bin/normalized.tiff");*/

		// Adding data regions to region template
		RegionTemplate *rt = new RegionTemplate();
		rt->setName("tile");
		rt->insertDataRegion(ddr2d);
		rt->insertDataRegion(ddr2dRefMask);
		//rt->insertDataRegion(ddr2dRefNorm);

		// Adding region template instance to collection
		rtCollection->addRT(rt);
	}

	return rtCollection;
}
void buildParameterSet(ParameterSet &normalization, ParameterSet &segmentation, std::string parametersFile){

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
		cout << "Unable to open file"; 
		exit(1);
	}

	std::vector<ArgumentBase*> targetMeanOptions;

	// add normalization parameters.
	ArgumentFloatArray *targetMeanAux = new ArgumentFloatArray(ArgumentFloat(-0.451225340366));
	targetMeanAux->addArgValue(ArgumentFloat(-0.0219714958221));
	targetMeanAux->addArgValue(ArgumentFloat(0.0335194170475));
	targetMeanOptions.push_back(targetMeanAux);

	/*ArgumentFloatArray *targetMeanAux2 = new ArgumentFloatArray(ArgumentFloat(-0.376));
	targetMeanAux2->addArgValue(ArgumentFloat(-0.0133));
	targetMeanAux2->addArgValue(ArgumentFloat(0.0243));
	targetMeanOptions.push_back(targetMeanAux2);*/

	normalization.addArguments(targetMeanOptions);

	assert(parameters.find("blue") != parameters.end());
	assert(parameters.find("green") != parameters.end());
	assert(parameters.find("red") != parameters.end());
	assert(parameters.find("T1") != parameters.end());
	assert(parameters.find("T2") != parameters.end());
	assert(parameters.find("G1") != parameters.end());
	assert(parameters.find("G2") != parameters.end());
	assert(parameters.find("minSize") != parameters.end());
	assert(parameters.find("maxSize") != parameters.end());
	assert(parameters.find("minSizePl") != parameters.end());
	assert(parameters.find("minSizeSeg") != parameters.end());
	assert(parameters.find("maxSizeSeg") != parameters.end());
	assert(parameters.find("fillHoles") != parameters.end());
	assert(parameters.find("recon") != parameters.end());
	assert(parameters.find("water") != parameters.end());


	// Blue channel
//	segmentation.addArgument(new ArgumentInt(220));
	segmentation.addRangeArguments(parameters["blue"][0], parameters["blue"][1], parameters["blue"][2]);

	// Green channel
	//segmentation.addArgument(new ArgumentInt(220));
	//segmentation.addRangeArguments(220, 240, 50);
	segmentation.addRangeArguments(parameters["green"][0], parameters["green"][1], parameters["green"][2]);

	// Red channel
	//segmentation.addArgument(new ArgumentInt(220));
	//segmentation.addRangeArguments(220, 240, 50);
	segmentation.addRangeArguments(parameters["red"][0], parameters["red"][1], parameters["red"][2]);

	// T1, T2  Red blood cell detection thresholds
	//segmentation.addArgument(new ArgumentFloat(5.0));// T1
	segmentation.addArgument(new ArgumentFloat(parameters["T1"][0]));// T1
	//segmentation.addArgument(new ArgumentFloat(4.0));// T2
	segmentation.addArgument(new ArgumentFloat(parameters["T2"][0]));// T2

	/*std::vector<ArgumentBase*> T1, T2;
	for(float i = 2.0; i <= 5; i+=0.5){
		T1.push_back(new ArgumentFloat(i+0.5));
		T2.push_back(new ArgumentFloat(i));
	}
	parSet.addArguments(T1);
	parSet.addArguments(T2);*/

	// G1 = 80, G2 = 45	-> Thresholds used to identify peak values after reconstruction. Those peaks
	// are preliminary areas in the calculation of the nuclei candidate set
	//segmentation.addArgument(new ArgumentInt(80));
	segmentation.addArgument(new ArgumentInt(parameters["G1"][0]));

	//segmentation.addArgument(new ArgumentInt(45));
	segmentation.addArgument(new ArgumentInt(parameters["G2"][0]));
	//segmentation.addRangeArguments(45, 70, 40);


	// minSize=11, maxSize=1000	-> Thresholds used to filter out preliminary nuclei areas that are not within a given size range  after peak identification
	//segmentation.addArgument(new ArgumentInt(11));
	segmentation.addArgument(new ArgumentInt(parameters["minSize"][0]));
	//parSet.addRangeArguments(10, 30, 5);
	//segmentation.addArgument(new ArgumentInt(1000));
	segmentation.addArgument(new ArgumentInt(parameters["maxSize"][0]));
	//parSet.addRangeArguments(900, 1500, 50);

	// int minSizePl=30 -> Filter out objects smaller than this value after overlapping objects are separate (watershed+other few operations)
	//segmentation.addArgument(new ArgumentInt(30));
	segmentation.addArgument(new ArgumentInt(parameters["minSizePl"][0]));

	//int minSizeSeg=21, int maxSizeSeg=1000 -> Perform final threshold on object sizes after objects are identified
	//segmentation.addArgument(new ArgumentInt(21));
	segmentation.addArgument(new ArgumentInt(parameters["minSizeSeg"][0]));
	//parSet.addRangeArguments(10, 30, 5);
	segmentation.addArgument(new ArgumentInt(parameters["maxSizeSeg"][0]));
	//segmentation.addArgument(new ArgumentInt(1000));
	//parSet.addRangeArguments(900, 1500, 50);

	// fill holes element 4-8
	int fill = (parameters["fillHoles"][0] < 6) ? 4 : 8;
	segmentation.addArgument(new ArgumentInt(fill));
	//segmentation.addArgument(new ArgumentInt(4));
	//parSet.addRangeArguments(4, 8, 4);

	// recon element 4-8

	int recon = (parameters["recon"][0] < 6) ? 4 : 8;
	segmentation.addArgument(new ArgumentInt(recon));
	//segmentation.addArgument(new ArgumentInt(8));
	//parSet.addRangeArguments(4, 8, 4);

	// watershed element 4-8

	int water = (parameters["water"][0] < 6) ? 4 : 8;
	segmentation.addArgument(new ArgumentInt(water));
	//segmentation.addArgument(new ArgumentInt(8));
	//parSet.addRangeArguments(4, 8, 4);

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
	sysEnv.startupSystem(argc, argv, "libcomponentnsdiffstudy.so");

	// Create region templates description without instantiating data
	rtCollection = RTFromFiles(inputFolderPath);

	ParameterSet parSetNormalization, parSetSegmentation;
	buildParameterSet(parSetNormalization, parSetSegmentation, parametersFile);

	int segCount = 0;
	// Build application dependency graph
	// Instantiate application dependency graph
	for(int i = 0; i < rtCollection->getNumRTs(); i++){

		int previousSegCompId = 0;

		// each tile computed with a different parameter combination results
		// into a region template w/ a different version value
		int versionNorm = 0;

		parSetNormalization.resetIterator();
		parSetSegmentation.resetIterator();

		// walk through parameter set and instantiate the computation pipeline for each parameter combination
		std::vector<ArgumentBase*> argSetInstanceNorm = parSetNormalization.getNextArgumentSetInstance();

		// while it has not finished w/ all parameter combinations
		while((argSetInstanceNorm).size() != 0){
			int versionSeg = 0;

			DiffMaskComp *diff = NULL;
			NormalizationComp *norm = new NormalizationComp();

			// normalization parameters
			norm->addArgument(new ArgumentInt(versionNorm));
			norm->addArgument(argSetInstanceNorm[0]);
			norm->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
			sysEnv.executeComponent(norm);

			std::vector<ArgumentBase*> argSetInstanceSegmentation = parSetSegmentation.getNextArgumentSetInstance();

			while((argSetInstanceSegmentation).size() != 0){

				Segmentation *seg = new Segmentation();

				// version of the data region read. Each parameter instance in norm creates a output w/ different version
				seg->addArgument(new ArgumentInt(versionNorm));

				// version of the data region outputed by the segmentation stage
				seg->addArgument(new ArgumentInt(versionSeg));

				// add remaining (application specific) parameters from the argSegInstance
				for(int j = 0; j < 15; j++)
					seg->addArgument(argSetInstanceSegmentation[j]);

				seg->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
				seg->addDependency(norm->getId());

				std::cout<< "Creating DiffMask" << std::endl;
				diff = new DiffMaskComp();

				// version of the data region that will be read. It is created during the segmentation.
				diff->addArgument(new ArgumentInt(versionSeg));

					// region template name
				diff->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
				diff->addDependency(previousSegCompId);
				diff->addDependency(seg->getId());
				// add to the list of diff component ids.
				diffComponentIds.push_back(diff->getId());

				sysEnv.executeComponent(seg);
				sysEnv.executeComponent(diff);

				std::cout << "Manager CompId: " << diff->getId() << " fileName: " << rtCollection->getRT(i)->getDataRegion(0)->getInputFileName() << std::endl;
				argSetInstanceSegmentation = parSetSegmentation.getNextArgumentSetInstance();
				segCount++;
				versionSeg++;
			}
			versionNorm++;
			parSetSegmentation.resetIterator();
			argSetInstanceNorm = parSetNormalization.getNextArgumentSetInstance();
		}
	}

	// End Creating Dependency Graph
	sysEnv.startupExecution();

	int diffPixels = 0;
	int foregroundPixels = 0;
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



