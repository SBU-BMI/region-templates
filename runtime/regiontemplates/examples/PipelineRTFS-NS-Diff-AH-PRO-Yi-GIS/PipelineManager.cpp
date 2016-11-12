#include "SysEnv.h"
#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <string>
#include "FileUtils.h"
#include "RegionTemplate.h"
#include "RegionTemplateCollection.h"

//#include "NormalizationComp.h"
#include "Segmentation.h"
#include "DiffMaskComp.h"
#include "DiceNotCoolMaskComp.h"
#include "ParameterSet.h"

#include "hclient.h"

#define INF    100000

namespace patch {
    template<typename T>
    std::string to_string(const T &n) {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }
}

void parseInputArguments(int argc, char **argv, std::string &inputFolder, std::string &AHpolicy,
                         std::string &initPercent) {
    // Used for parameters parsing
    for (int i = 0; i < argc - 1; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'i') {
            inputFolder = argv[i + 1];
        }
        if (argv[i][0] == '-' && argv[i][1] == 'o') {
            initPercent = argv[i + 1];
        }
        if (argv[i][0] == '-' && argv[i][1] == 'f') {
            AHpolicy = argv[i + 1];
        }
    }
}


RegionTemplateCollection *RTFromFiles(std::string inputFolderPath) {
    // Search for input files in folder path
    FileUtils fileUtils(".mask.png");
    std::vector<std::string> fileList;
    fileUtils.traverseDirectoryRecursive(inputFolderPath, fileList);
    RegionTemplateCollection *rtCollection = new RegionTemplateCollection();
    rtCollection->setName("inputimage");

    std::cout << "Input Folder: " << inputFolderPath << std::endl;

    std::string temp;
    // Create one region template instance for each input data file
    // (creates representations without instantiating them)
    for (int i = 0; i < fileList.size(); i++) {

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

int main(int argc, char **argv) {
    int numClients = 1;

    // Folder when input data images are stored
    std::string inputFolderPath, AHpolicy = "nm.so", initPercent;
    std::vector<RegionTemplate *> inputRegionTemplates;
    RegionTemplateCollection *rtCollection;
    std::vector<int> segComponentIds[numClients];
    std::vector<int> diffComponentIds[numClients];
    std::vector<int> diceNotCoolComponentIds[numClients];
    std::map<std::string, double> perfDataBase;

    std::vector<hdesc_t *> hdesc;

    parseInputArguments(argc, argv, inputFolderPath, AHpolicy, initPercent);

    // Handler to the distributed execution system environment
    SysEnv sysEnv;

    // Tell the system which libraries should be used
    sysEnv.startupSystem(argc, argv, "libcomponentnsdiffahproyigis.so");

    // Create region templates description without instantiating data
    rtCollection = RTFromFiles(inputFolderPath);


    // AH SETUP //
    /* Initialize a Harmony client. */

    for (int i = 0; i < numClients; i++) {
        hdesc.push_back(harmony_init(&argc, &argv));
        if (hdesc[i] == NULL) {
            fprintf(stderr, "Failed to initialize a harmony session.\n");
            return -1;
        }
    }

    char name[1024];

    snprintf(name, sizeof(name), "Pipeline-NS-AH-PRO-YI-GIS.%d", getpid());


    if (harmony_session_name(hdesc[0], name) != 0) {
        fprintf(stderr, "Could not set session name.\n");
        return -1;
    }


    if (
            harmony_real(hdesc[0], "otsuRatio", 0.1, 2.5, 0.1) != 0
            || harmony_real(hdesc[0], "curvatureWeight", 0.0, 1.0, 0.05) != 0
            || harmony_real(hdesc[0], "sizeThld", 1, 20, 1) != 0
            || harmony_real(hdesc[0], "sizeUpperThld", 50, 400, 5) != 0
            || harmony_real(hdesc[0], "mpp", 0.25, 0.25, 0.05) != 0
            || harmony_real(hdesc[0], "mskernel", 5, 30, 1) != 0
            || harmony_int(hdesc[0], "levelSetNumberOfIteration", 5, 150, 1) != 0
            ) {
        fprintf(stderr, "Failed to define tuning session\n");
        return -1;
    }

    harmony_strategy(hdesc[0], AHpolicy.c_str());
    //harmony_strategy(hdesc[0], "pro.so");
//	 harmony_strategy(hdesc[0], "nm.so");
    if (initPercent.size() > 0) {
        harmony_setcfg(hdesc[0], "INIT_PERCENT", initPercent.c_str());
        std::cout << "AH configuration: " << AHpolicy << " INIT_PERCENT: " << initPercent << std::endl;
    }
    std::cout << endl << "-------------------------- AH configuration: " << AHpolicy <<
    " -------------------------- " << std::endl;
    char numbuf[12];
    snprintf(numbuf, sizeof(numbuf), "%d", numClients);

    harmony_setcfg(hdesc[0], "CLIENT_COUNT", numbuf);

    printf("Starting Harmony...\n");
    if (harmony_launch(hdesc[0], NULL, 0) != 0) {
        fprintf(stderr,
                "Could not launch tuning session: %s. E.g. export HARMONY_HOME=$HOME/region-templates/runtime/build/regiontemplates/external-src/activeharmony-4.5/\n",
                harmony_error_string(hdesc[0]));

        return -1;
    }

    double otsuRatio[numClients], curvatureWeight[numClients], sizeThld[numClients], sizeUpperThld[numClients], mpp[numClients], msKernel[numClients];
    long levelSetNumberOfIteration[numClients];

    for (int i = 0; i < numClients; i++) {
        otsuRatio[i] = curvatureWeight[i] = sizeThld[i] = sizeUpperThld[i] = mpp[i] = msKernel[i] = 0.0;
        levelSetNumberOfIteration[i] = 0;
    }

    for (int i = 0; i < numClients; i++) {
        /* Bind the session variables to local variables. */
        if (harmony_bind_real(hdesc[i], "otsuRatio", &otsuRatio[i]) != 0
            || harmony_bind_real(hdesc[i], "curvatureWeight", &curvatureWeight[i]) != 0
            || harmony_bind_real(hdesc[i], "sizeThld", &sizeThld[i]) != 0
            || harmony_bind_real(hdesc[i], "sizeUpperThld", &sizeUpperThld[i]) != 0
            || harmony_bind_real(hdesc[i], "mpp", &mpp[i]) != 0
            || harmony_bind_real(hdesc[i], "mskernel", &msKernel[i]) != 0
            || harmony_bind_int(hdesc[i], "levelSetNumberOfIteration", &levelSetNumberOfIteration[i]) != 0) {
            fprintf(stderr, "Failed to register variable\n");
            harmony_fini(hdesc[i]);
            return -1;
        }
        printf("Bind %d successful\n", i);
    }


    /* Join this client to the tuning session we established above. */
    for (int i = 0; i < numClients; i++) {
        if (harmony_join(hdesc[i], NULL, 0, name) != 0) {
            fprintf(stderr, "Could not connect to harmony server: %s\n",
                    harmony_error_string(hdesc[i]));
            harmony_fini(hdesc[i]);
            return -1;
        }
    }
    // END AH SETUP //

    double perf[numClients];

    int max_number_of_iterations = 50;
    float *totaldiffs = (float *) malloc(sizeof(float) * max_number_of_iterations);
    uint64_t *totalexecutiontimes = (uint64_t *) malloc(sizeof(uint64_t) * max_number_of_iterations);
    double *totalExecutionTimesNormalized = (double *) malloc(sizeof(double) * max_number_of_iterations);
    float maxdiff = 0;
    double minperf = INT_MAX;

    double timeWeight = 1;
    double metricWeight = 2;

    int versionSeg = 0;
    bool executedAlready[numClients];

    /* main loop */
    for (int loop = 0; !harmony_converged(hdesc[0]) && loop < max_number_of_iterations;) {

        for (int i = 0; i < numClients; i++) {
            perf[i] = INF;

            int hresult;
            do {
                hresult = harmony_fetch(hdesc[i]);
            } while (hresult == 0);


            if (hresult < 0) {
                fprintf(stderr, "Failed to fetch values from server: %s\n",
                        harmony_error_string(hdesc[i]));
                harmony_fini(hdesc[i]);
                return -1;
            }

            std::ostringstream oss;
            oss << otsuRatio[i] << "-" << curvatureWeight[i] << "-" << sizeThld[i] << "-" << sizeUpperThld[i] << "-" <<
            mpp[i] << "-" << msKernel[i] << "-" << levelSetNumberOfIteration[i];

            // if not found in performance database
            if (perfDataBase.find(oss.str()) != perfDataBase.end()) {
                perf[i] = perfDataBase.find(oss.str())->second;
                std::cout << "Parameters already tested: " << oss.str() << " perf: " << perf[i] << std::endl;
                executedAlready[i] = true;
            } else {
                executedAlready[i] = false;
            }

        }

        int segCount = 0;
        // Build application dependency graph
        // Instantiate application dependency graph
        for (int i = 0; i < rtCollection->getNumRTs(); i++) {

            int previousSegCompId = 0;

            for (int j = 0; j < numClients; j++) {

                if (executedAlready[j] == false) {

                    std::cout << "BEGIN: LoopIdx: " << loop << "[" << otsuRatio[i] << "-" << curvatureWeight[i] <<
                    "-" << sizeThld[i] << "-" << sizeUpperThld[i] << "-" << mpp[i] << "-" << msKernel[i] << "-" <<
                    levelSetNumberOfIteration[i] << "]" << std::endl;

                    // Creating segmentation component
                    Segmentation *seg = new Segmentation();

                    // version of the data region generated by the segmentation stage
                    seg->addArgument(new ArgumentInt(versionSeg));

                    // add remaining (application specific) parameters from the argSegInstance
                    seg->addArgument(new ArgumentFloat(otsuRatio[j]));
                    seg->addArgument(new ArgumentFloat(curvatureWeight[j]));
                    seg->addArgument(new ArgumentFloat(sizeThld[j]));
                    seg->addArgument(new ArgumentFloat(sizeUpperThld[j]));
                    seg->addArgument(new ArgumentFloat(mpp[j]));
                    seg->addArgument(new ArgumentFloat(msKernel[j]));
                    seg->addArgument(new ArgumentInt(levelSetNumberOfIteration[j]));


                    // and region template instance that it is suppose to process
                    seg->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());

                    std::cout << "Creating DiffMask" << std::endl;
                    DiffMaskComp *diff = new DiffMaskComp();
                    DiceNotCoolMaskComp *diceNotCool = new DiceNotCoolMaskComp();

                    // version of the data region that will be read. It is created during the segmentation.
                    diff->addArgument(new ArgumentInt(versionSeg));
                    diceNotCool->addArgument(new ArgumentInt(versionSeg));

                    // region template name
                    diff->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
                    diff->addDependency(seg->getId());
                    diceNotCool->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
                    diceNotCool->addDependency(diff->getId());

                    // add to the list of diff component ids.
                    segComponentIds[j].push_back(seg->getId());
                    diffComponentIds[j].push_back(diff->getId());
                    diceNotCoolComponentIds[j].push_back(diceNotCool->getId());

                    sysEnv.executeComponent(seg);
                    sysEnv.executeComponent(diff);
                    sysEnv.executeComponent(diceNotCool);

                    std::cout << "Manager CompId: " << diff->getId() << " fileName: " <<
                    rtCollection->getRT(i)->getDataRegion(0)->getInputFileName() << std::endl;
                    segCount++;
                    versionSeg++;

                }
            }

        }
        // End Creating Dependency Graph
        sysEnv.startupExecution();

        for (int j = 0; j < numClients; j++) {
            float diff = 0;
            float secondaryMetric = 0;
            float diceNotCoolValue = 0;

            if (executedAlready[j] == false) {
                for (int i = 0; i < diffComponentIds[j].size(); i++) {
                    char *resultData = sysEnv.getComponentResultData(diffComponentIds[j][i]);
                    char *diceNotCoolResultData = sysEnv.getComponentResultData(diceNotCoolComponentIds[j][i]);
                    std::cout << "Diff Id: " << diffComponentIds[j][i] << " \tresultData: ";
                    if (resultData != NULL) {
                        std::cout << "size: " << ((int *) resultData)[0] << " \thadoopgis-metric: " <<
                        ((float *) resultData)[1] <<
                        " \tsecondary: " << ((float *) resultData)[2] << " \tdiceNotCool: " <<
                        ((float *) diceNotCoolResultData)[1] << std::endl;
                        diff += ((float *) resultData)[1];
                        secondaryMetric += ((float *) resultData)[2];
                        diceNotCoolValue += ((float *) diceNotCoolResultData)[1];
                    } else {
                        std::cout << "NULL" << std::endl;
                    }
                    char *segExecutionTime = sysEnv.getComponentResultData(segComponentIds[j][i]);
                    if (segExecutionTime != NULL) {
                        totalexecutiontimes[loop] = ((int *) segExecutionTime)[1];
                        cout << "Segmentation execution time:" << totalexecutiontimes[loop] << endl;
                    }
                    sysEnv.eraseResultData(diffComponentIds[j][i]);
                    sysEnv.eraseResultData(segComponentIds[j][i]);
                }
                diffComponentIds[j].clear();
                segComponentIds[j].clear();
                diceNotCoolComponentIds[j].clear();

//                float dicePlusDiceNotCool = ( diff + diceNotCoolValue);
//                diff = dicePlusDiceNotCool/2;
//                if (diff <= 0) diff = FLT_EPSILON;

                //Multi Objective Tuning
                double tSlowest = 750000; //Empirical Data
                double tFastest = 10000; //Empirical Data
                double timeNormalized = (tSlowest - (double) totalexecutiontimes[loop]) / (tSlowest - tFastest);
//TODO Change here if using PixelCompare or Hadoopgis
                perf[j] = (double) 1 / diff; //If using Hadoopgis
                //perf[j] = diff; //If using PixelCompare.
                //perf[j] = (double) 1 / (double) (metricWeight * diff + timeWeight * timeNormalized); //Multi Objective Tuning
                cout << "$$$$$$$$$$$$$$$$ - Diff: " << diff << " Time Normalized: " << timeNormalized << " Time: " <<
                totalexecutiontimes[loop] << " Perf: " << perf[j] << endl;
                totaldiffs[loop] = diff;
                if (perf[j] < 0) perf[j] = 0;

                (maxdiff < diff) ? maxdiff = diff : maxdiff;
                (minperf > perf[j]) ? minperf = perf[j] : minperf;
                totalExecutionTimesNormalized[loop] = timeNormalized;
                std::ostringstream oss;
                oss << otsuRatio[j] << "-" << curvatureWeight[j] << "-" << sizeThld[j] << "-" << sizeUpperThld[j];

                perfDataBase[oss.str()] = perf[j];

                std::cout << "END: LoopIdx: " << loop - numClients + 1 << " otsuRatio: " << otsuRatio[j] <<
                " curvatureWeight: " << curvatureWeight[j]
                << " sizeThld: " << sizeThld[j] << " sizeUpperThld: " << sizeUpperThld[j] << " mpp: " << mpp[j] <<
                " msKernel: " << msKernel[j] << " levelSetNumberOfIteration: " << levelSetNumberOfIteration[j] <<
                " total diff: " << diff <<
                " secondaryMetric: " <<
                secondaryMetric << " perf: " << perf[j] << std::endl;
                loop++;
            }

            // Report the performance we've just measured.
            if (harmony_report(hdesc[j], perf[j]) != 0) {
                fprintf(stderr, "Failed to report performance to server.\n");
                harmony_fini(hdesc[j]);
                return -1;
            }
        }
    }


    if (harmony_converged(hdesc[0])) {
        std::cout << "\t\tOptimization loop has converged!!!!" << std::endl;
    }
    else {
        std::cout << "\t\tThe tuning algorithm did not converge" << std::endl;
    }
    for (int i = 0; i < max_number_of_iterations; ++i) {
        double perfWeighted =
                (double) 1 / (double) (metricWeight * totaldiffs[i] + timeWeight * totalExecutionTimesNormalized[i]);
        if (perfWeighted < 0) perfWeighted = 0;
        std::cout << "\t\tLoop: " << i << " \tDiff: " << totaldiffs[i] << "\tExecution Time: " <<
        totalexecutiontimes[i] << "\tTime Normalized: " << totalExecutionTimesNormalized[i] << " \tPerf(weighted): " <<
        perfWeighted << std::endl;
    }
    std::cout << "\tMaxDiff: " << maxdiff << std::endl;
    std::cout << "\tBest answer - MinPerfWeighted: " << minperf << std::endl;

    // Finalize all processes running and end execution
    sysEnv.finalizeSystem();

    delete rtCollection;

    return 0;
}



