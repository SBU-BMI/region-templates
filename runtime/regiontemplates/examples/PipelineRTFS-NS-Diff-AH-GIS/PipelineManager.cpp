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
    // Folder when input data images are stored
    std::string inputFolderPath, AHpolicy = "nm.so", initPercent;
    std::vector<RegionTemplate *> inputRegionTemplates;
    RegionTemplateCollection *rtCollection;
    std::vector<int> diffComponentIds;
    std::map<std::string, double> perfDataBase;

    hdesc_t *hdesc;

    parseInputArguments(argc, argv, inputFolderPath, AHpolicy, initPercent);

    // Handler to the distributed execution system environment
    SysEnv sysEnv;

    // Tell the system which libraries should be used
    sysEnv.startupSystem(argc, argv, "libcomponentnsdiffahgis.so");

    // Create region templates description without instantiating data
    rtCollection = RTFromFiles(inputFolderPath);


    // AH SETUP //
    /* Initialize a Harmony client. */
    hdesc = harmony_init(&argc, &argv);
    if (hdesc == NULL) {
        fprintf(stderr, "Failed to initialize a harmony session.\n");
        return -1;
    }
    char name[1024];

    snprintf(name, sizeof(name), "Pipeline-NS-AH.%d", getpid());


    if (harmony_session_name(hdesc, name) != 0) {
        fprintf(stderr, "Could not set session name.\n");
        return -1;
    }

    //Tell AH which params we are going to use and their range.
    if (harmony_int(hdesc, "blue", 210, 240, 10) != 0
        || harmony_int(hdesc, "green", 210, 240, 10) != 0
        || harmony_int(hdesc, "red", 210, 240, 10) != 0
        || harmony_real(hdesc, "T1", 2.5, 7.5, 0.5) != 0
        || harmony_real(hdesc, "T2", 2.5, 7.5, 0.5) != 0
        || harmony_int(hdesc, "G1", 5, 80, 5) != 0
        || harmony_int(hdesc, "minSize", 2, 40, 2) != 0
        || harmony_int(hdesc, "maxSize", 900, 1500, 50) != 0
        || harmony_int(hdesc, "G2", 2, 40, 2) != 0
        || harmony_int(hdesc, "minSizePl", 5, 80, 5) != 0
        || harmony_int(hdesc, "minSizeSeg", 2, 40, 2) != 0
        || harmony_int(hdesc, "maxSizeSeg", 900, 1500, 50) != 0
        || harmony_int(hdesc, "fillHoles", 4, 8, 4) != 0
        || harmony_int(hdesc, "recon", 4, 8, 4) != 0
        || harmony_int(hdesc, "watershed", 4, 8, 4) != 0
            ) {
        //|| harmony_int(hdesc, "G1", 60, 90, 1) != 0)
        fprintf(stderr, "Failed to define tuning session\n");
        return -1;
    }

    harmony_strategy(hdesc, AHpolicy.c_str());
    if (initPercent.size() > 0) {
        harmony_setcfg(hdesc, "INIT_PERCENT", initPercent.c_str());
        std::cout << "AH configuration: " << AHpolicy << " INIT_PERCENT: " << initPercent << std::endl;
    }

    printf("Starting Harmony...\n");
    if (harmony_launch(hdesc, NULL, 0) != 0) {
        fprintf(stderr,
                "Could not launch tuning session: %s. E.g. export HARMONY_HOME=$HOME/region-templates/runtime/build/regiontemplates/external-src/activeharmony-4.5/\n",
                harmony_error_string(hdesc));

        return -1;
    }

    long blue, green = 220, red = 220;
    double T1, T2;
    long G1, G2;
    long minSize = 11, maxSize = 1000, fillHolesElement = 4, mophElement = 8, watershedElement = 8;
    long minSizePl;
    long minSizeSeg = 21, maxSizeSeg = 1000;

    /* Bind the session variables to local variables. */
    if (harmony_bind_int(hdesc, "blue", &blue) != 0
        || harmony_bind_int(hdesc, "green", &green) != 0
        || harmony_bind_int(hdesc, "red", &red) != 0
        || harmony_bind_real(hdesc, "T1", &T1) != 0
        || harmony_bind_real(hdesc, "T2", &T2) != 0
        || harmony_bind_int(hdesc, "G1", &G1) != 0
        || harmony_bind_int(hdesc, "minSize", &minSize) != 0
        || harmony_bind_int(hdesc, "maxSize", &maxSize) != 0
        || harmony_bind_int(hdesc, "G2", &G2) != 0
        || harmony_bind_int(hdesc, "minSizePl", &minSizePl) != 0
        || harmony_bind_int(hdesc, "minSizeSeg", &minSizeSeg) != 0
        || harmony_bind_int(hdesc, "maxSizeSeg", &maxSizeSeg) != 0
        || harmony_bind_int(hdesc, "fillHoles", &fillHolesElement) != 0
        || harmony_bind_int(hdesc, "recon", &mophElement) != 0
        || harmony_bind_int(hdesc, "watershed", &watershedElement) != 0
            ) {
        fprintf(stderr, "Failed to register variable\n");
        harmony_fini(hdesc);
        return -1;
    }

    /* Join this client to the tuning session we established above. */
    if (harmony_join(hdesc, NULL, 0, name) != 0) {
        fprintf(stderr, "Could not connect to harmony server: %s\n",
                harmony_error_string(hdesc));
        harmony_fini(hdesc);
        return -1;
    }

    // END AH SETUP //

    double perf = 1000;

    int versionNorm = 0, versionSeg = 0;
    /* main loop */
    for (int loop = 1; !harmony_converged(hdesc) && loop <= 100 && perf > 0; ++loop) {

        int hresult;
        //busy waiting
        do {
            hresult = harmony_fetch(hdesc);
        } while (hresult == 0);


        if (hresult < 0) {
            fprintf(stderr, "Failed to fetch values from server: %s\n",
                    harmony_error_string(hdesc));
            harmony_fini(hdesc);
            return -1;
        }

        std::ostringstream oss;
        oss << blue << "-" << green << "-" << red << "-" << T1 << "-" << T2 << "-" << G1 << "-" << minSize << "-" <<
        maxSize <<
        "-" << G2 << "-" << minSizePl << "-" << minSizeSeg << "-" << maxSizeSeg << "-" << fillHolesElement << "-" <<
        mophElement << "-" << watershedElement;
        if (perfDataBase.find(oss.str()) == perfDataBase.end()) {
            ParameterSet parSetNormalization, parSetSegmentation;
            buildParameterSet(parSetNormalization, parSetSegmentation);

            std::cout << "BEGIN: LoopIdx: " << loop << " blue: " << blue << " green: " << green << " red: " << red <<
            " T1: " << T1 << " T2: " << T2 << " G1: " << G1 << " G2: " << G2 << " minSize: " << minSize <<
            " maxSize: " << maxSize << " minSizePl: " << minSizePl << " minSizeSeg: " << minSizeSeg <<
            " maxSizeSeg: " << maxSizeSeg << " fillHolesElement: " << fillHolesElement << " morphElement: " <<
            mophElement <<
            " watershedElement: " << watershedElement << std::endl;

            int segCount = 0;
            // Build application dependency graph
            // Instantiate application dependency graph
            for (int i = 0; i < rtCollection->getNumRTs(); i++) {

                int previousSegCompId = 0;

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

                Segmentation *seg = new Segmentation();

                // version of the data region red. Each parameter instance in norm creates a output w/ different version
                seg->addArgument(new ArgumentInt(versionNorm));

                // version of the data region outputed by the segmentation stage
                seg->addArgument(new ArgumentInt(versionSeg));

                // add remaining (application specific) parameters from the argSegInstance
                seg->addArgument(new ArgumentInt(blue));
                seg->addArgument(new ArgumentInt(green));
                seg->addArgument(new ArgumentInt(red));
                seg->addArgument(new ArgumentFloat(T1));
                seg->addArgument(new ArgumentFloat(T2));
                seg->addArgument(new ArgumentInt(G1));
                seg->addArgument(new ArgumentInt(G2));

                // not been varied yet
                seg->addArgument(new ArgumentInt(minSize));
                seg->addArgument(new ArgumentInt(maxSize));
                seg->addArgument(new ArgumentInt(minSizePl));
                seg->addArgument(new ArgumentInt(minSizeSeg));
                seg->addArgument(new ArgumentInt(maxSizeSeg));
                seg->addArgument(new ArgumentInt(fillHolesElement));
                seg->addArgument(new ArgumentInt(mophElement));
                seg->addArgument(new ArgumentInt(watershedElement));

                seg->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
                seg->addDependency(norm->getId());

                std::cout << "Creating DiffMask" << std::endl;
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

                std::cout << "Manager CompId: " << diff->getId() << " fileName: " <<
                rtCollection->getRT(i)->getDataRegion(0)->getInputFileName() << std::endl;
                //			argSetInstanceSegmentation = parSetSegmentation.getNextArgumentSetInstance();
                segCount++;

                //			parSetSegmentation.resetIterator();
                //			argSetInstanceNorm = parSetNormalization.getNextArgumentSetInstance();

            }
            versionSeg++;
            versionNorm++;
            // End Creating Dependency Graph
            sysEnv.startupExecution();

            float diff = 0;
            float secondaryMetric = 0;
            for (int i = 0; i < diffComponentIds.size(); i++) {
                char *resultData = sysEnv.getComponentResultData(diffComponentIds[i]);
                std::cout << "Diff Id: " << diffComponentIds[i] << " resultData: ";
                if (resultData != NULL) {
                    std::cout << "size: " << ((int *) resultData)[0] << " diffPixels: " << ((float *) resultData)[1] <<
                    " refPixels: " << ((float *) resultData)[2] << std::endl;
                    diff += ((float *) resultData)[1];
                    secondaryMetric += ((float *) resultData)[2];
                } else {
                    std::cout << "NULL" << std::endl;
                }
                sysEnv.eraseResultData(diffComponentIds[i]);
            }
            diffComponentIds.clear();
            //TODO Change here if using PixelCompare or Hadoopgis
            perf = (double) 1 / diff; //If using Hadoopgis
            //perf = diff; //If using PixelCompare.
            std::cout << "END: LoopIdx: " << loop << " blue: " << blue << " green: " << green << " red: " << red <<
            " T1: " << T1 << " T2: " << T2 << " G1: " << G1 << " G2: " << G2 << " minSize: " << minSize <<
            " maxSize: " << maxSize << " minSizePl: " << minSizePl << " minSizeSeg: " << minSizeSeg <<
            " maxSizeSeg: " << maxSizeSeg << " fillHolesElement: " << fillHolesElement << " morphElement: " <<
            mophElement <<
            " watershedElement: " << watershedElement << " total hadoopgis diff: " << diff << " secondaryMetric: " <<
            secondaryMetric << " perf: " << perf << std::endl;

            std::cout << "Perf: " << perf << std::endl;
            perfDataBase[oss.str()] = perf;
        } else {
            perf = perfDataBase.find(oss.str())->second;
            std::cout << "Parameters already tested: " << oss.str() << " perf: " << perf << std::endl;
            loop--;
        }

        // Report the performance we've just measured.
        if (harmony_report(hdesc, perf) != 0) {
            fprintf(stderr, "Failed to report performance to server.\n");
            harmony_fini(hdesc);
            return -1;
        }

        sleep(2);
    }

    if (harmony_converged(hdesc)) {
        std::cout << "\t\tOptimization loop has converged!!!!" << std::endl;
    }
    // Finalize all processes running and end execution
    sysEnv.finalizeSystem();

    delete rtCollection;

    return 0;
}



