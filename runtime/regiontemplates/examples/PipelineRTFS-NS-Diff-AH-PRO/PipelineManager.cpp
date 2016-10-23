
#include "SysEnv.h"
#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <regiontemplates/autotuning/activeharmony/NealderMeadTuning.h>
#include <regiontemplates/autotuning/geneticalgorithm/GeneticAlgorithm.h>
#include "FileUtils.h"
#include "RegionTemplate.h"
#include "RegionTemplateCollection.h"

#include "NormalizationComp.h"
#include "Segmentation.h"
#include "FeatureExtraction.h"
#include "DiffMaskComp.h"
#include "ParameterSet.h"

#include "hclient.h"

#define INF	100000

namespace patch{
	template < typename T > std::string to_string(const T& n)
	{
		std::ostringstream stm;
		stm << n;
		return stm.str();
	}
}
void parseInputArguments(int argc, char**argv, std::string &inputFolder, std::string &AHpolicy, std::string &initPercent){
	// Used for parameters parsing
	for(int i = 0; i < argc-1; i++){
		if(argv[i][0] == '-' && argv[i][1] == 'i'){
			inputFolder = argv[i+1];
		}
		if(argv[i][0] == '-' && argv[i][1] == 'o'){
			initPercent = argv[i+1];
		}
		if(argv[i][0] == '-' && argv[i][1] == 'f'){
			AHpolicy = argv[i+1];
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
void buildParameterSet(ParameterSet &normalization, ParameterSet &segmentation){

	std::vector<ArgumentBase*> targetMeanOptions;

	ArgumentFloatArray *targetMeanAux = new ArgumentFloatArray(ArgumentFloat(-0.632356));
	targetMeanAux->addArgValue(ArgumentFloat(-0.0516004));
	targetMeanAux->addArgValue(ArgumentFloat(0.0376543));
// add normalization parameters.
//	ArgumentFloatArray *targetMeanAux = new ArgumentFloatArray(ArgumentFloat(-0.451225340366));
//	targetMeanAux->addArgValue(ArgumentFloat(-0.0219714958221));
//	targetMeanAux->addArgValue(ArgumentFloat(0.0335194170475));
	targetMeanOptions.push_back(targetMeanAux);

	/*ArgumentFloatArray *targetMeanAux2 = new ArgumentFloatArray(ArgumentFloat(-0.376));
	targetMeanAux2->addArgValue(ArgumentFloat(-0.0133));
	targetMeanAux2->addArgValue(ArgumentFloat(0.0243));
	targetMeanOptions.push_back(targetMeanAux2);*/

	normalization.addArguments(targetMeanOptions);

	// Blue channel
	//segmentation.addArgument(new ArgumentInt(220));
	segmentation.addRangeArguments(200, 240, 50);

	// Green channel
	//segmentation.addArgument(new ArgumentInt(220));
	segmentation.addRangeArguments(200, 240, 50);
	// Red channel
	//segmentation.addArgument(new ArgumentInt(220));
	segmentation.addRangeArguments(200, 240, 50);

	// T1, T2  Red blood cell detection thresholds
	segmentation.addArgument(new ArgumentFloat(5.0));// T1
	segmentation.addArgument(new ArgumentFloat(4.0));// T2
	/*std::vector<ArgumentBase*> T1, T2;
	for(float i = 2.0; i <= 5; i+=0.5){
		T1.push_back(new ArgumentFloat(i+0.5));
		T2.push_back(new ArgumentFloat(i));
	}
	parSet.addArguments(T1);
	parSet.addArguments(T2);*/

	// G1 = 80, G2 = 45	-> Thresholds used to identify peak values after reconstruction. Those peaks
	// are preliminary areas in the calculation of the nuclei candidate set
	segmentation.addArgument(new ArgumentInt(80));
// G2 = 45	-> Thresholds used to identify peak values after reconstruction. Those peaks
	segmentation.addArgument(new ArgumentInt(45));
	//segmentation.addRangeArguments(70, 70, 40);

	// minSize=11, maxSize=1000	-> Thresholds used to filter out preliminary nuclei areas that are not within a given size range  after peak identification
	segmentation.addArgument(new ArgumentInt(11));
	//parSet.addRangeArguments(10, 30, 5);
	segmentation.addArgument(new ArgumentInt(1000));
	//parSet.addRangeArguments(900, 1500, 50);



	// int minSizePl=30 -> Filter out objects smaller than this value after overlapping objects are separate (watershed+other few operations)
	segmentation.addArgument(new ArgumentInt(30));

	//int minSizeSeg=21, int maxSizeSeg=1000 -> Perform final threshold on object sizes after objects are identified
	segmentation.addArgument(new ArgumentInt(21));
	//parSet.addRangeArguments(10, 30, 5);
	segmentation.addArgument(new ArgumentInt(1000));
	//parSet.addRangeArguments(900, 1500, 50);

	// fill holes element 4-8
	segmentation.addArgument(new ArgumentInt(4));
	//parSet.addRangeArguments(4, 8, 4);

	// recon element 4-8
	segmentation.addArgument(new ArgumentInt(8));
	//parSet.addRangeArguments(4, 8, 4);

	// watershed element 4-8
	segmentation.addArgument(new ArgumentInt(8));
	//parSet.addRangeArguments(4, 8, 4);

	return;
}

int main (int argc, char **argv){

// Folder when input data images are stored
	std::string inputFolderPath, AHpolicy = "nm.so", initPercent;
	std::vector<RegionTemplate *> inputRegionTemplates;
	RegionTemplateCollection *rtCollection;
	parseInputArguments(argc, argv, inputFolderPath, AHpolicy, initPercent);

	int max_number_of_generations = 10;
	int mutationchance = 30;
	int crossoverrate = 50;
	int propagationamount = 2;

	//USING GA
	int numClients = 10; //popsize
	int max_number_of_tests = max_number_of_generations * numClients;


	//USING AH
//	int max_number_of_tests = 100;
//	int numClients = 1;


//	TuningInterface *tuningClient = new NealderMeadTuning(1, max_number_of_tests, numClients);
	TuningInterface *tuningClient = new GeneticAlgorithm(max_number_of_generations, numClients, mutationchance,
														 crossoverrate,
														 propagationamount,
														 1);


	std::vector<int> diffComponentIds[numClients];
    std::map<std::string, double> perfDataBase; //Checks if a param has been tested already


    std::vector<hdesc_t *> hdesc;



	// Handler to the distributed execution system environment
	SysEnv sysEnv;

	// Tell the system which libraries should be used
	sysEnv.startupSystem(argc, argv, "libcomponentnsdiffahpro.so");

	// Create region templates description without instantiating data
	rtCollection = RTFromFiles(inputFolderPath);


	// AH SETUP //
	 /* Initialize a Harmony client. */






	if (tuningClient->initialize(argc, argv) != 0) {
        fprintf(stderr, "Failed to initialize tuning session.\n");
        return -1;
    };

	if (tuningClient->declareParam("blue", 210, 240, 10) != 0 ||
		tuningClient->declareParam("green", 210, 240, 10) != 0 ||
		tuningClient->declareParam("red", 210, 240, 10) != 0 ||
		tuningClient->declareParam("T1", 2.5, 7.5, 0.5) != 0 ||
		tuningClient->declareParam("T2", 2.5, 7.5, 0.5) != 0 ||
		tuningClient->declareParam("G1", 5, 80, 5) != 0 ||
		tuningClient->declareParam("minSize", 2, 40, 2) != 0 ||
		tuningClient->declareParam("maxSize", 900, 1500, 50) != 0 ||
		tuningClient->declareParam("G2", 2, 40, 2) != 0 ||
		tuningClient->declareParam("minSizePl", 5, 80, 5) != 0 ||
		tuningClient->declareParam("minSizeSeg", 2, 40, 2) != 0 ||
		tuningClient->declareParam("maxSizeSeg", 900, 1500, 50) != 0 ||
		tuningClient->declareParam("fillHoles", 4, 8, 4) != 0 ||
		tuningClient->declareParam("recon", 4, 8, 4) != 0 ||
		tuningClient->declareParam("watershed", 4, 8, 4) != 0) {
            fprintf(stderr, "Failed to define tuning session\n");
            return -1;
        }

	if (tuningClient->configure() != 0) {
        fprintf(stderr, "Failed to initialize tuning session.\n");
        return -1;
    };

	double perf[numClients];


	float *totaldiffs = (float *) malloc(sizeof(float) * max_number_of_tests);
	float mindiff = std::numeric_limits<float>::infinity();;
	float minperf = std::numeric_limits<float>::infinity();;

	int versionNorm = 0, versionSeg = 0;
	bool executedAlready[numClients];

	/* main loop */
    for (; !tuningClient->hasConverged();) {

        cout << "ITERATION: " << tuningClient->getIteration() << endl;

		//Get new param suggestions from the tuning client
		tuningClient->fetchParams();
		//Apply fitness function for each individual
		for(int i = 0; i < numClients; i++){
			perf[i] = INF;

            std::ostringstream oss;
            oss << "PARAMS";
            typedef std::map<std::string, double *>::iterator it_type;
            for (it_type iterator = tuningClient->getParamSet(i)->paramSet.begin();
                 iterator != tuningClient->getParamSet(i)->paramSet.end(); iterator++) {
                //iterator.first key
                //iterator.second value
				oss << " - " << iterator->first << ": " << *(iterator->second);
            }

            // / if not found in performance database
			if(perfDataBase.find(oss.str()) != perfDataBase.end()){
				perf[i] = perfDataBase.find(oss.str())->second;
				std::cout << "Parameters already tested: "<< oss.str() << " perf: "<< perf<< std::endl;

				executedAlready[i] = true;
			}else{
				executedAlready[i] = false;
			}

		}

		//for(int i =0; i < numClients; i++){

		ParameterSet parSetNormalization, parSetSegmentation;
		buildParameterSet(parSetNormalization, parSetSegmentation);



		int segCount = 0;
		// Build application dependency graph
		// Instantiate application dependency graph
		for(int i = 0; i < rtCollection->getNumRTs(); i++){

			int previousSegCompId = 0;

			// CREATE NORMALIZATION STEP
			// each tile computed with a different parameter combination results
			// into a region template w/ a different version value
			parSetNormalization.resetIterator();

			// walk through parameter set and instantiate the computation pipeline for each parameter combination
			std::vector<ArgumentBase*> argSetInstanceNorm = parSetNormalization.getNextArgumentSetInstance();

			DiffMaskComp *diff = NULL;
			NormalizationComp *norm = new NormalizationComp();

			// normalization parameters
			norm->addArgument(new ArgumentInt(versionNorm));
			norm->addArgument(argSetInstanceNorm[0]);
			norm->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
			sysEnv.executeComponent(norm);
			// END CREATING NORMALIZATION STEP

			for(int j = 0; j < numClients; j++){

				if(executedAlready[j] == false){

					std::cout << "BEGIN: LoopIdx: " << tuningClient->getIteration() * numClients + (j);

					typedef std::map<std::string, double *>::iterator it_type;
					for (it_type iterator = tuningClient->getParamSet(j)->paramSet.begin();
						 iterator != tuningClient->getParamSet(j)->paramSet.end(); iterator++) {
						//iterator.first key
						//iterator.second value
						std::cout << " - " << iterator->first << ": " << *(iterator->second);
					}

					std::cout << std::endl;

					Segmentation *seg = new Segmentation();

					// version of the data region red. Each parameter instance in norm creates a output w/ different version
					seg->addArgument(new ArgumentInt(versionNorm));

					// version of the data region generated by the segmentation stage
					seg->addArgument(new ArgumentInt(versionSeg));

					// add remaining (application specific) parameters from the argSegInstance
                    seg->addArgument(new ArgumentInt(
                            (int) round(tuningClient->getParamValue("blue", j))));
                    seg->addArgument(new ArgumentInt(
                            (int) round(tuningClient->getParamValue("green", j))));
                    seg->addArgument(new ArgumentInt(
                            (int) round(tuningClient->getParamValue("red", j))));

                    seg->addArgument(
                            new ArgumentFloat((float) (tuningClient->getParamValue("T1", j))));
                    seg->addArgument(
                            new ArgumentFloat((float) (tuningClient->getParamValue("T2", j))));

                    seg->addArgument(new ArgumentInt(
                            (int) round(tuningClient->getParamValue("G1", j))));
                    seg->addArgument(new ArgumentInt(
                            (int) round(tuningClient->getParamValue("G2", j))));
                    seg->addArgument(new ArgumentInt(
                            (int) round(tuningClient->getParamValue("minSize", j))));
                    seg->addArgument(new ArgumentInt(
                            (int) round(tuningClient->getParamValue("maxSize", j))));
                    seg->addArgument(new ArgumentInt(
                            (int) round(tuningClient->getParamValue("minSizePl", j))));
                    seg->addArgument(new ArgumentInt(
                            (int) round(tuningClient->getParamValue("minSizeSeg", j))));
                    seg->addArgument(new ArgumentInt(
                            (int) round(tuningClient->getParamValue("maxSizeSeg", j))));
                    seg->addArgument(new ArgumentInt(
                            (int) round(tuningClient->getParamValue("fillHoles", j))));
                    seg->addArgument(new ArgumentInt(
                            (int) round(tuningClient->getParamValue("recon", j))));
                    seg->addArgument(new ArgumentInt(
                            (int) round(tuningClient->getParamValue("watershed", j))));

					seg->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
					seg->addDependency(norm->getId());

					std::cout<< "Creating DiffMask" << std::endl;
					diff = new DiffMaskComp();

					// version of the data region that will be read. It is created during the segmentation.
					diff->addArgument(new ArgumentInt(versionSeg));

					// region template name
					diff->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
					diff->addDependency(seg->getId());

					// add to the list of diff component ids.
					diffComponentIds[j].push_back(diff->getId());

					sysEnv.executeComponent(seg);
					sysEnv.executeComponent(diff);

					std::cout << "Manager CompId: " << diff->getId() << " fileName: " << rtCollection->getRT(i)->getDataRegion(0)->getInputFileName() << std::endl;
					segCount++;
					versionSeg++;

				}
			}

		}
		versionNorm++;
		// End Creating Dependency Graph
		sysEnv.startupExecution();

		for(int j = 0; j < numClients; j++){
            float diff = 0;
            float secondaryMetric = 0;

			std::ostringstream oss;
			oss << "PARAMS";
			typedef std::map<std::string, double *>::iterator it_type;
			for (it_type iterator = tuningClient->getParamSet(j)->paramSet.begin();
				 iterator != tuningClient->getParamSet(j)->paramSet.end(); iterator++) {
				//iterator.first key
				//iterator.second value
				oss << " - " << iterator->first << ": " << *(iterator->second);
			}

			if(executedAlready[j] ==  false){

				//Read the values from the Diff Component stage
				for(int i = 0; i < diffComponentIds[j].size(); i++){
					char * resultData = sysEnv.getComponentResultData(diffComponentIds[j][i]);
					std::cout << "Diff Id: "<< diffComponentIds[j][i] << " resultData: ";
					if(resultData != NULL){
                        std::cout << "size: " << ((int *) resultData)[0] << " diffPixels: " <<
                        ((float *) resultData)[1] <<
                        " refPixels: " << ((float *) resultData)[2] << std::endl;
                        diff += ((float *) resultData)[1];
                        secondaryMetric += ((float *) resultData)[2];
					}else{
						std::cout << "NULL" << std::endl;
					}
					sysEnv.eraseResultData(diffComponentIds[j][i]);
				}
				diffComponentIds[j].clear();
				perf[j] = diff; //If using PixelCompare.
				//perf[j] = 1/diff; //If using HadoopGIS


				std::cout << "END: LoopIdx: " << tuningClient->getIteration() * numClients + (j);
                typedef std::map<std::string, double *>::iterator it_type;
                for (it_type iterator = tuningClient->getParamSet(j)->paramSet.begin();
                     iterator != tuningClient->getParamSet(j)->paramSet.end(); iterator++) {
                    //iterator.first key
                    //iterator.second value
                    std::cout << " - " << iterator->first << ": " << *(iterator->second);
                }

                std::cout << " total diff: " << diff << " secondaryMetric: " <<
                secondaryMetric << " perf: " << perf[j] << std::endl;


				totaldiffs[tuningClient->getIteration() * numClients + (j)] = diff;
				if (minperf > perf[j]) {
					minperf = perf[j];
					mindiff = diff;
				}


				perfDataBase[oss.str()] = perf[j];

			} else {
				perf[j] = perfDataBase[oss.str()];

				std::cout << "ATTENTION! Param set executed already:" << std::endl;
				std::cout << "END: LoopIdx: " << tuningClient->getIteration() * numClients + (j);
				std::cout << oss.str() << endl;

				std::cout << " perf: " << perf[j] << std::endl;
			}

			// Report the performance we've just measured.
            tuningClient->reportScore(perf[j], j);
		}


		bool shouldIterate = false;
		for (int k = 0; k < numClients; ++k) {
			shouldIterate |= !executedAlready[k];
		}
		if (shouldIterate == true) tuningClient->nextIteration();
	}
	sleep(2);


    std::cout << "\t\tResults:" << std::endl;
	for (int i = 0; i < max_number_of_tests; ++i) {
		std::cout << "\t\tTest: " << i << " Diff: " << totaldiffs[i] << std::endl;
    }
	std::cout << "\tBest Diff: " << mindiff << std::endl;


	// Finalize all processes running and end execution
	sysEnv.finalizeSystem();

	delete rtCollection;

	return 0;
}



