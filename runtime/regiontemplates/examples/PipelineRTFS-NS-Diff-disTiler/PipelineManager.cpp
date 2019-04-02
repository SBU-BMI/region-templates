#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <string>

#include "SysEnv.h"
#include "RegionTemplate.h"
#include "BoundingBox.h"

#include "NormalizationComp.h"
#include "Segmentation.h"
#include "DiffMaskComp.h"

#include "openslide.h"
// #include <glib.h>

RegionTemplate* getInputRT(std::string path) {
    DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
    ddr2d->setName("initial");
    ddr2d->setId("initial");
    ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
    ddr2d->setIsAppInput(true);
    ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
    ddr2d->setInputFileName(path);

    RegionTemplate *rt = new RegionTemplate();
    rt->insertDataRegion(ddr2d);

    return rt;
}

NormalizationComp* genNormalization(int versionNorm, RegionTemplate* rt) {
    // Normalization args
    ArgumentFloatArray *targetMeanAux = new ArgumentFloatArray();
    targetMeanAux->addArgValue(ArgumentFloat(-0.632356));
    targetMeanAux->addArgValue(ArgumentFloat(-0.0516004));
    targetMeanAux->addArgValue(ArgumentFloat(0.0376543));

    // Instantiate the pipeline
    NormalizationComp* norm = new NormalizationComp();
    norm->addArgument(new ArgumentInt(versionNorm));
    norm->addArgument(targetMeanAux);
    norm->addRegionTemplateInstance(rt, rt->getName());

    return norm;
}

Segmentation* genSegmentation(int versionSeg, int versionNorm, 
    int normId, RegionTemplate* rt) {

    Segmentation* seg = new Segmentation();
    
    // Version of the data region read. Each parameter instance 
    //   in norm creates a output w/ different version
    seg->addArgument(new ArgumentInt(versionNorm));
    
    // Version of the data region outputted by the segmentation stage
    seg->addArgument(new ArgumentInt(versionSeg));

    // Add remaining (application specific) parameters:
    seg->addArgument(new ArgumentInt(220));   // Blue channel
    seg->addArgument(new ArgumentInt(220));   // Green channel
    seg->addArgument(new ArgumentInt(220));   // Red channel
    seg->addArgument(new ArgumentFloat(5.0)); // T1
    seg->addArgument(new ArgumentFloat(4.0)); // T2
    seg->addArgument(new ArgumentInt(80));    // G1
    seg->addArgument(new ArgumentInt(45));    // G2
    seg->addArgument(new ArgumentInt(11));    // minSize
    seg->addArgument(new ArgumentInt(1000));  // maxSize
    seg->addArgument(new ArgumentInt(30));    // minSizePl
    seg->addArgument(new ArgumentInt(21));    // minSizeSeg
    seg->addArgument(new ArgumentInt(1000));  // maxSizeSeg
    seg->addArgument(new ArgumentInt(4));     // fill holes element
    seg->addArgument(new ArgumentInt(8));     // recon element
    seg->addArgument(new ArgumentInt(8));     // watershed

    seg->addRegionTemplateInstance(rt, rt->getName());
    seg->addDependency(normId);

    return seg;
}

DiffMaskComp* genDiffMaskComp(int versionSeg, int segId, 
    RegionTemplate* rtIn, RegionTemplate* rtMask) {

    DiffMaskComp* diff = new DiffMaskComp();
    // version of the data region that will be read. 
    // It is created during the segmentation.
    diff->addArgument(new ArgumentInt(versionSeg));

    // region template name
    diff->addRegionTemplateInstance(rtIn, rtIn->getName());
    diff->addRegionTemplateInstance(rtMask, rtMask->getName());
    diff->addDependency(segId);

    return diff;
}

int findArgPos(string s, int argc, char** argv) {
    for (int i=1; i<argc; i++)
        if (string(argv[i]).compare(s)==0)
            return i;
    return -1;
}

int32_t getLargestLevel(openslide_t *osr) {
    int32_t levels = openslide_get_level_count(osr);
    int64_t w, h;

    int64_t maxSize = -1;
    int32_t maxLevel = -1;

    for (int32_t l=0; l<levels; l++) {
        openslide_get_level_dimensions(osr, l, &w, &h);
        if (h*w > maxSize) {
            maxSize = h*w;
            maxLevel = l;
        }
    }

    return maxLevel;
}

void osrRegionToCVMat(openslide_t* osr, int xi, int yi, 
    int xo, int yo, int level, cv::Mat& thisTile) {

    uint32_t* osrRegion = new uint32_t[(xo-xi)*(yo-yi)];
    openslide_read_region(osr, osrRegion, xi, yi, level, (xo-xi), (yo-yi));

    thisTile = cv::Mat((yo-yi), (xo-xi), CV_8UC3, cv::Scalar(0, 0, 0));
    int64_t numOfPixelPerTile = thisTile.total();

    for (int64_t it = 0; it < numOfPixelPerTile; ++it) {
        uint32_t p = osrRegion[it];

        uint8_t a = (p >> 24) & 0xFF;
        uint8_t r = (p >> 16) & 0xFF;
        uint8_t g = (p >> 8) & 0xFF;
        uint8_t b = p & 0xFF;

        switch (a) {
            case 0:
                r = 0;
                b = 0;
                g = 0;
                break;
            case 255:
                // no action needed
                break;
            default:
                r = (r * 255 + a / 2) / a;
                g = (g * 255 + a / 2) / a;
                b = (b * 255 + a / 2) / a;
                break;
        }

        // write back
        thisTile.at<cv::Vec3b>(it)[0] = b;
        thisTile.at<cv::Vec3b>(it)[1] = g;
        thisTile.at<cv::Vec3b>(it)[2] = r;
    }

    delete[] osrRegion;

    return;
}

int main (int argc, char **argv){
    // Handler to the distributed execution system environment
    SysEnv sysEnv;

    // Tell the system which libraries should be used
    sysEnv.startupSystem(argc, argv, "libcomponentnsdiffdt.so");

    // get args
    if (argc < 5) {
        std::cout << "Usage: ./exec [-t <sqr_dim>] -i <input_basename> "
            << "-p <path>" << std::endl;
        std::cout << "\t-t: Break images into sqr_dim sized squares for "
            << "execution, i.e., don't use disTiler" << std::endl;
        std::cout << "\t-i: Root name of the input images. There must be "
            << "at least two images: <basename>.tiff and <basename>.mask.tiff" 
            << std::endl;
        std::cout << "\t-p: Path of all input images" << std::endl;
        exit(0);
    }

    // Regular tiling?
    int tSize = 0;
    if (findArgPos("-t", argc, argv) != -1) {
        tSize = atoi(argv[findArgPos("-t", argc, argv)+1]);
    }

    // Input images
    string imgBasename;
    if (findArgPos("-i", argc, argv) == -1) {
        cout << "Missing image basename." << endl;
        return 0;
    } else {
        imgBasename = argv[findArgPos("-i", argc, argv)+1];
    }

    // Input images
    string inputFolderPath;
    if (findArgPos("-p", argc, argv) == -1) {
        cout << "Missing input images path." << endl;
        return 0;
    } else {
        inputFolderPath = argv[findArgPos("-p", argc, argv)+1];
    }

    // Instantiate the initial region template with the input image
    std::string imgFilePath = inputFolderPath + imgBasename + ".tiff";
    RegionTemplate* inputRT = getInputRT(imgFilePath);
    inputRT->setName("img");

    // Instantiate the initial region template with the input mask image
    std::string maskFilePath = inputFolderPath + imgBasename + ".mask.tiff";
    RegionTemplate* maskRT = getInputRT(maskFilePath);
    maskRT->setName("mask");

    std::vector<int> diffComponentIds;
    if (tSize == 0) {
        // get parameters from the pipeline
        int versionNorm=0; // REMOVAL CANDIDATE------------------------------->
        int versionSeg=0; // REMOVAL CANDIDATE-------------------------------->

        // Instantiate stages
        NormalizationComp* norm = genNormalization(versionNorm, inputRT);
        Segmentation* seg = genSegmentation(versionSeg, 
            versionNorm, norm->getId(), inputRT);
        DiffMaskComp* diff = genDiffMaskComp(versionSeg, 
            seg->getId(), inputRT, maskRT);
        diffComponentIds.push_back(diff->getId());

        // add stages to execution
        sysEnv.executeComponent(norm);
        sysEnv.executeComponent(seg);
        sysEnv.executeComponent(diff);
    } else {

        imgFilePath = inputFolderPath + imgBasename + ".svs";
        openslide_t *osr = openslide_open(imgFilePath.c_str());

        int32_t maxLevel = getLargestLevel(osr);
        int xi = 300, yi = 150;
        int xo = 5500, yo = 4000;
        cv::Mat tile;
        osrRegionToCVMat(osr, xi, yi, xo, yo, maxLevel, tile);

        cv::imwrite("tileTest.png", tile);

        return -1;







        // break initial images into tiles
        DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
        ddr2d->setName("tile0");
        ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
        ddr2d->setTimestamp(0);
        BoundingBox b;
        Point p;
        ddr2d->setROI(BoundingBox(Point(0,0,0), Point(200,200,2)));
        ddr2d->insertBB2IdElement(ddr2d->getROI(), "initial");
        // ddr2d->insertBB2IdElement(ddr2d->getROI(), "tile0-bb2ide");
        ddr2d->setId("initial");
        inputRT->insertDataRegion(ddr2d);


        // get parameters from the pipeline
        int versionNorm=0; // REMOVAL CANDIDATE------------------------------->
        int versionSeg=0; // REMOVAL CANDIDATE-------------------------------->

        // Instantiate stages
        NormalizationComp* norm = genNormalization(versionNorm, inputRT);
        Segmentation* seg = genSegmentation(versionSeg, 
            versionNorm, norm->getId(), inputRT);
        DiffMaskComp* diff = genDiffMaskComp(versionSeg, 
            seg->getId(), inputRT, maskRT);
        diffComponentIds.push_back(diff->getId());

        // add stages to execution
        sysEnv.executeComponent(norm);
        sysEnv.executeComponent(seg);
        sysEnv.executeComponent(diff);
    }



    // End Creating Dependency Graph
    sysEnv.startupExecution();

    float diff_f = 0;
    float secondaryMetric = 0;
    for(int i = 0; i < diffComponentIds.size(); i++){
        char *resultData = sysEnv.getComponentResultData(diffComponentIds[i]);
        std::cout << "Diff Id: " << diffComponentIds[i] << " resultData -  ";
        if(resultData != NULL){
            std::cout << "size: " << ((int *) resultData)[0] 
                << " Diff: " << ((float *) resultData)[1] 
                << " Secondary Metric: " << ((float *) resultData)[2] 
                << std::endl;
            diff_f += ((float *) resultData)[1];
            secondaryMetric += ((float *) resultData)[2];
        }else{
            std::cout << "NULL" << std::endl;
        }
    }
    std::cout << "Total diff: " << diff_f << " Secondary Metric: " 
        << secondaryMetric << std::endl;

    // Finalize all processes running and end execution
    sysEnv.finalizeSystem();

    return 0;
}



