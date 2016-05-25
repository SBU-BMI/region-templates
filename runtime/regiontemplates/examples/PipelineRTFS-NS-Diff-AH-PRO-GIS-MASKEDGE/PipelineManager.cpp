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

void buildParameterSet(ParameterSet &normalization, ParameterSet &segmentation) {

    std::vector<ArgumentBase *> targetMeanOptions;

//    ArgumentFloatArray *targetMeanAux = new ArgumentFloatArray(ArgumentFloat(-0.632356));
//    targetMeanAux->addArgValue(ArgumentFloat(-0.0516004));
//    targetMeanAux->addArgValue(ArgumentFloat(0.0376543));
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

    //segmentation.addArgument(new ArgumentInt(45));
    segmentation.addRangeArguments(70, 70, 40);


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

int main(int argc, char **argv) {
    int numClients = 1;

    // Folder when input data images are stored
    std::string inputFolderPath, AHpolicy = "pro.so", initPercent;
    std::vector<RegionTemplate *> inputRegionTemplates;
    RegionTemplateCollection *rtCollection;
    std::vector<int> diffComponentIds[numClients];
    std::map<std::string, double> perfDataBase;

    std::vector<hdesc_t *> hdesc;

    parseInputArguments(argc, argv, inputFolderPath, AHpolicy, initPercent);

    // Handler to the distributed execution system environment
    SysEnv sysEnv;

    // Tell the system which libraries should be used
    sysEnv.startupSystem(argc, argv, "libcomponentnsdiffahprogisedge.so");

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

    snprintf(name, sizeof(name), "Pipeline-NS-AH-PRO.%d", getpid());


    if (harmony_session_name(hdesc[0], name) != 0) {
        fprintf(stderr, "Could not set session name.\n");
        return -1;
    }

    for (int i = 0; i < numClients; i++) {
        if (harmony_int(hdesc[i], "blue", 210, 240, 10) != 0
            || harmony_int(hdesc[i], "green", 210, 240, 10) != 0
            || harmony_int(hdesc[i], "red", 210, 240, 10) != 0
            || harmony_real(hdesc[i], "T1", 2.5, 7.5, 0.5) != 0
            || harmony_real(hdesc[i], "T2", 2.5, 7.5, 0.5) != 0
            || harmony_int(hdesc[i], "G1", 5, 80, 5) != 0
            || harmony_int(hdesc[i], "minSize", 2, 40, 2) != 0
            || harmony_int(hdesc[i], "maxSize", 900, 1500, 50) != 0
            || harmony_int(hdesc[i], "G2", 2, 40, 2) != 0
            || harmony_int(hdesc[i], "minSizePl", 5, 80, 5) != 0
            || harmony_int(hdesc[i], "minSizeSeg", 2, 40, 2) != 0
            || harmony_int(hdesc[i], "maxSizeSeg", 900, 1500, 50) != 0
            || harmony_int(hdesc[i], "fillHoles", 4, 8, 4) != 0
            || harmony_int(hdesc[i], "recon", 4, 8, 4) != 0
            || harmony_int(hdesc[i], "watershed", 4, 8, 4) != 0
                ) {
            fprintf(stderr, "Failed to define tuning session\n");
            return -1;
        }
    }

    harmony_strategy(hdesc[0], "pro.so");
    if (initPercent.size() > 0) {
        harmony_setcfg(hdesc[0], "INIT_PERCENT", initPercent.c_str());
        std::cout << "AH configuration: " << AHpolicy << " INIT_PERCENT: " << initPercent << std::endl;
    }
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

    long blue[numClients], green[numClients], red[numClients];
    double T1[numClients], T2[numClients];
    long G1[numClients], G2[numClients];
    long minSize[numClients], maxSize[numClients], fillHolesElement[numClients], morphElement[numClients], watershedElement[numClients];
    long minSizePl[numClients];
    long minSizeSeg[numClients], maxSizeSeg[numClients];

    for (int i = 0; i < numClients; i++) {
        blue[i] = 220;
        green[i] = 220;
        red[i] = 220;
        T1[i] = 5.0;
        T2[i] = 4.0;
        G1[i] = 80;
        G2[i] = 45;
        minSize[i] = 11;
        maxSize[i] = 1000;
        fillHolesElement[i] = 4;
        morphElement[i] = 8;
        watershedElement[i] = 8;
        minSizePl[i] = minSize[i] + 5;
        minSizeSeg[i] = 21;
        maxSizeSeg[i] = 1000;
    }

    for (int i = 0; i < numClients; i++) {
        /* Bind the session variables to local variables. */
        if (harmony_bind_int(hdesc[i], "blue", &blue[i]) != 0
            || harmony_bind_int(hdesc[i], "green", &green[i]) != 0
            || harmony_bind_int(hdesc[i], "red", &red[i]) != 0
            || harmony_bind_real(hdesc[i], "T1", &T1[i]) != 0
            || harmony_bind_real(hdesc[i], "T2", &T2[i]) != 0
            || harmony_bind_int(hdesc[i], "G1", &G1[i]) != 0
            || harmony_bind_int(hdesc[i], "minSize", &minSize[i]) != 0
            || harmony_bind_int(hdesc[i], "maxSize", &maxSize[i]) != 0
            || harmony_bind_int(hdesc[i], "G2", &G2[i]) != 0
            || harmony_bind_int(hdesc[i], "minSizePl", &minSizePl[i]) != 0
            || harmony_bind_int(hdesc[i], "minSizeSeg", &minSizeSeg[i]) != 0
            || harmony_bind_int(hdesc[i], "maxSizeSeg", &maxSizeSeg[i]) != 0
            || harmony_bind_int(hdesc[i], "fillHoles", &fillHolesElement[i]) != 0
            || harmony_bind_int(hdesc[i], "recon", &morphElement[i]) != 0
            || harmony_bind_int(hdesc[i], "watershed", &watershedElement[i]) != 0
                ) {
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



    double perf[numClients];//=100000;

    int max_number_of_iterations = 100;
    float *totaldiffs = (float *) malloc(sizeof(float) * max_number_of_iterations);
    float maxdiff = 0;

    int versionNorm = 0, versionSeg = 0;
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
            // SOME PARAMS CAN BE A FUNCTION OF OTHER PARAMS!
            //            T2[i] = T1[i] - 1.0;
            //            G2[i] = G1[i] - 35;
            //            minSizePl[i] = minSize[i] + 5;


            std::ostringstream oss;
            oss << blue[i] << "-" << green[i] << "-" << red[i] << "-" << T1[i] << "-" << T2[i] << "-" << G1[i] << "-" <<
            minSize[i] << "-" << maxSize[i] <<
            "-" << G2[i] << "-" << minSizePl[i] << "-" << minSizeSeg[i] << "-" << maxSizeSeg[i] << "-" <<
            fillHolesElement[i] << "-" << morphElement[i] << "-" << watershedElement[i];
            // if not found in performance database
            if (perfDataBase.find(oss.str()) != perfDataBase.end()) {
                perf[i] = perfDataBase.find(oss.str())->second;
                std::cout << "Parameters already tested: " << oss.str() << " perf: " << perf << std::endl;

                executedAlready[i] = true;
            } else {
                executedAlready[i] = false;
            }

        }

        //for(int i =0; i < numClients; i++){

        ParameterSet parSetNormalization, parSetSegmentation;
        buildParameterSet(parSetNormalization, parSetSegmentation);

        int segCount = 0;
        // Build application dependency graph
        // Instantiate application dependency graph
        for (int i = 0; i < rtCollection->getNumRTs(); i++) {

            int previousSegCompId = 0;

            // CREATE NORMALIZATION STEP
            // each tile computed with a different parameter combination results
            // into a region template w/ a different version value
            parSetNormalization.resetIterator();

            // walk through parameter set and instantiate the computation pipeline for each parameter combination
            std::vector<ArgumentBase *> argSetInstanceNorm = parSetNormalization.getNextArgumentSetInstance();

            DiffMaskComp *diff = NULL;
            NormalizationComp *norm = new NormalizationComp();

            // normalization parameters
            norm->addArgument(new ArgumentInt(versionNorm));
            norm->addArgument(argSetInstanceNorm[0]);
            norm->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
            sysEnv.executeComponent(norm);
            // END CREATING NORMALIZATION STEP

            for (int j = 0; j < numClients; j++) {

                if (executedAlready[j] == false) {
                    std::cout << "BEGIN: LoopIdx: " << loop << " blue: " << blue[j] << " green: " << green[j] <<
                    " red: " << red[j] <<
                    " T1: " << T1[j] << " T2: " << T2[j] << " G1: " << G1[j] << " G2: " << G2[j] << " minSize: " <<
                    minSize[j] <<
                    " maxSize: " << maxSize[j] << " minSizePl: " << minSizePl[j] << " minSizeSeg: " << minSizeSeg[j] <<
                    " maxSizeSeg: " << maxSizeSeg[j] << " fillHolesElement: " << fillHolesElement[j] <<
                    " morphElement: " << morphElement[j] <<
                    " watershedElement: " << watershedElement[j] << std::endl;

                    Segmentation *seg = new Segmentation();

                    // version of the data region red. Each parameter instance in norm creates a output w/ different version
                    seg->addArgument(new ArgumentInt(versionNorm));

                    // version of the data region generated by the segmentation stage
                    seg->addArgument(new ArgumentInt(versionSeg));

                    // add remaining (application specific) parameters from the argSegInstance
                    seg->addArgument(new ArgumentInt(blue[j]));
                    seg->addArgument(new ArgumentInt(green[j]));
                    seg->addArgument(new ArgumentInt(red[j]));
                    seg->addArgument(new ArgumentFloat(T1[j]));
                    seg->addArgument(new ArgumentFloat(T2[j]));
                    seg->addArgument(new ArgumentInt(G1[j]));
                    seg->addArgument(new ArgumentInt(G2[j]));

                    // not been varied yet
                    seg->addArgument(new ArgumentInt(minSize[j]));
                    seg->addArgument(new ArgumentInt(maxSize[j]));
                    seg->addArgument(new ArgumentInt(minSizePl[j]));
                    seg->addArgument(new ArgumentInt(minSizeSeg[j]));
                    seg->addArgument(new ArgumentInt(maxSizeSeg[j]));
                    seg->addArgument(new ArgumentInt(fillHolesElement[j]));
                    seg->addArgument(new ArgumentInt(morphElement[j]));
                    seg->addArgument(new ArgumentInt(watershedElement[j]));

                    seg->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
                    seg->addDependency(norm->getId());

                    std::cout << "Creating DiffMask" << std::endl;
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

                    std::cout << "Manager CompId: " << diff->getId() << " fileName: " <<
                    rtCollection->getRT(i)->getDataRegion(0)->getInputFileName() << std::endl;
                    segCount++;
                    versionSeg++;

                }
            }

        }
        versionNorm++;
        // End Creating Dependency Graph
        sysEnv.startupExecution();

        for (int j = 0; j < numClients; j++) {
            float diff = 0;
            float secondaryMetric = 0;

            if (executedAlready[j] == false) {
                for (int i = 0; i < diffComponentIds[j].size(); i++) {
                    char *resultData = sysEnv.getComponentResultData(diffComponentIds[j][i]);
                    std::cout << "Diff Id: " << diffComponentIds[j][i] << " resultData: ";
                    if (resultData != NULL) {
                        std::cout << "size: " << ((int *) resultData)[0] << " diffPixels: " <<
                        ((float *) resultData)[1] <<
                        " refPixels: " << ((float *) resultData)[2] << std::endl;
                        diff += ((float *) resultData)[1];
                        secondaryMetric += ((float *) resultData)[2];
                    } else {
                        std::cout << "NULL" << std::endl;
                    }
                    sysEnv.eraseResultData(diffComponentIds[j][i]);
                }
                diffComponentIds[j].clear();
                //TODO Change here if using PixelCompare or Hadoopgis
                perf[j] = (double) 1 / diff; //If using Hadoopgis
                //perf[j] = diff; //If using PixelCompare.
                std::cout << "END: LoopIdx: " << loop << " blue: " << blue[j] << " green: " << green[j] <<
                " red: " << red[j] <<
                " T1: " << T1[j] << " T2: " << T2[j] << " G1: " << G1[j] << " G2: " << G2[j] << " minSize: " <<
                minSize[j] <<
                " maxSize: " << maxSize[j] << " minSizePl: " << minSizePl[j] << " minSizeSeg: " << minSizeSeg[j] <<
                " maxSizeSeg: " << maxSizeSeg[j] << " fillHolesElement: " << fillHolesElement[j] << " morphElement: " <<
                morphElement[j] <<
                " watershedElement: " << watershedElement[j] <<
                " total hadoopgis diff: " << diff << " secondaryMetric: " <<
                secondaryMetric << " perf: " << perf[j] << std::endl;

                totaldiffs[loop] = diff;
                (maxdiff < diff) ? maxdiff = diff : maxdiff;

                std::ostringstream oss;
                oss << blue[j] << "-" << green[j] << "-" << red[j] << "-" << T1[j] << "-" << T2[j] << "-" << G1[j] <<
                "-" << minSize[j] << "-" << maxSize[j] <<
                "-" << G2[j] << "-" << minSizePl[j] << "-" << minSizeSeg[j] << "-" << maxSizeSeg[j] << "-" <<
                fillHolesElement[j] << "-" << morphElement[j] << "-" << watershedElement[j];


                perfDataBase[oss.str()] = perf[j];
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
    sleep(2);


    if (harmony_converged(hdesc[0])) {
        std::cout << "\t\tOptimization loop has converged!!!!" << std::endl;

        for (int i = 0; i < max_number_of_iterations; ++i) {
            std::cout << "\t\tLoop: " << i << " Diff: " << totaldiffs[i] << std::endl;
        }
        std::cout << "\tMaxDiff: " << maxdiff << std::endl;
    }
    else {
        std::cout << "\t\tThe tuning algorithm did not converge" << std::endl;

        for (int i = 0; i < max_number_of_iterations; ++i) {
            std::cout << "\t\tLoop: " << i << " Diff: " << totaldiffs[i] << std::endl;
        }
        std::cout << "\tMaxDiff: " << maxdiff << std::endl;
    }

    // Finalize all processes running and end execution
    sysEnv.finalizeSystem();

    delete rtCollection;

    return 0;
}



