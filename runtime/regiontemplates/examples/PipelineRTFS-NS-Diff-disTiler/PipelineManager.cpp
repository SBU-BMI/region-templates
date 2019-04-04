#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <math.h>

#include "SysEnv.h"
#include "RegionTemplate.h"
#include "BoundingBox.h"

#include "NormalizationComp.h"
#include "Segmentation.h"
#include "DiffMaskComp.h"

#include "openslide.h"

static const std::string IN_RT_NAME = "img";
static const std::string MASK_RT_NAME = "mask";
static const std::string REF_DDR_NAME = "initial";

void osrRegionToCVMat(openslide_t* osr, int xi, int yi, 
    int xo, int yo, int level, cv::Mat& thisTile) {

    uint32_t* osrRegion = new uint32_t[(xo-xi+1)*(yo-yi+1)];
    openslide_read_region(osr, osrRegion, xi, yi, level, (xo-xi+1), (yo-yi+1));

    thisTile = cv::Mat((yo-yi+1), (xo-xi+1), CV_8UC3, cv::Scalar(0, 0, 0));
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

RegionTemplate* getInputRT(std::string path) {
    DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
    ddr2d->setName(REF_DDR_NAME);
    ddr2d->setId(REF_DDR_NAME);
    ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
    ddr2d->setIsAppInput(true);
    ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
    ddr2d->setInputFileName(path);

    RegionTemplate *rt = new RegionTemplate();
    rt->insertDataRegion(ddr2d);

    return rt;
}

DenseDataRegion2D* getDDRTile(openslide_t* osr, int xi, int yi, 
    int xo, int yo, int level, std::string ddName, std::string ddrId) {

    DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
    ddr2d->setName(ddName);
    ddr2d->setId(ddrId);
    ddr2d->setInputType(DataSourceType::DATA_SPACES);
    ddr2d->setIsAppInput(true);
    ddr2d->setOutputType(DataSourceType::DATA_SPACES);

    // add cv::Mat to the new ddr2d
    cv::Mat tile;
    osrRegionToCVMat(osr, xi, yi, xo, yo, level, tile);

    ddr2d->setData(tile);

    return ddr2d;
}

NormalizationComp* genNormalization(RegionTemplate* rt, std::string ddrName) {

    // Normalization args
    ArgumentFloatArray *targetMeanAux = new ArgumentFloatArray();
    targetMeanAux->addArgValue(ArgumentFloat(-0.632356));
    targetMeanAux->addArgValue(ArgumentFloat(-0.0516004));
    targetMeanAux->addArgValue(ArgumentFloat(0.0376543));

    // Instantiate the pipeline
    NormalizationComp* norm = new NormalizationComp();
    norm->setIo(rt->getName(), ddrName);
    norm->addArgument(targetMeanAux);
    norm->addArgument(new ArgumentString(rt->getName()));
    norm->addArgument(new ArgumentString(ddrName));
    norm->addRegionTemplateInstance(rt, rt->getName());

    return norm;
}

Segmentation* genSegmentation(int normId, RegionTemplate* rt, 
    std::string ddrName) {

    Segmentation* seg = new Segmentation();

    seg->setIo(rt->getName(), ddrName);
    
    seg->addArgument(new ArgumentString(rt->getName()));
    seg->addArgument(new ArgumentString(ddrName));

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

DiffMaskComp* genDiffMaskComp(int segId, RegionTemplate* rtIn, 
    RegionTemplate* rtMask, std::string ddrName) {

    DiffMaskComp* diff = new DiffMaskComp();

    diff->setIo(rtIn->getName(), rtMask->getName(), ddrName);

    diff->addArgument(new ArgumentString(rtIn->getName()));
    diff->addArgument(new ArgumentString(rtMask->getName()));
    diff->addArgument(new ArgumentString(ddrName));


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
    int tSize = -1;
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

    std::vector<int> diffComponentIds;
    if (tSize == 0) { // no tiling
        // Instantiate the initial region template with the input image
        std::string imgFilePath = inputFolderPath + imgBasename + ".tiff";
        RegionTemplate* inputRT = getInputRT(imgFilePath);
        inputRT->setName(IN_RT_NAME);

        // Instantiate the initial region template with the input mask image
        std::string maskFilePath = inputFolderPath + imgBasename + ".mask.tiff";
        RegionTemplate* maskRT = getInputRT(maskFilePath);
        maskRT->setName(MASK_RT_NAME);

        // Instantiate stages
        NormalizationComp* norm = genNormalization(inputRT, REF_DDR_NAME);
        Segmentation* seg = genSegmentation(norm->getId(), 
            inputRT, REF_DDR_NAME);
        DiffMaskComp* diff = genDiffMaskComp(seg->getId(), 
            inputRT, maskRT, REF_DDR_NAME);
        diffComponentIds.push_back(diff->getId());

        // add stages to execution
        sysEnv.executeComponent(norm);
        sysEnv.executeComponent(seg);
        sysEnv.executeComponent(diff);

    } else if (tSize > 0) { // regular tiling
        // Instantiate the initial region template with the input image
        std::string imgFilePath = inputFolderPath + imgBasename + ".svs";
        openslide_t* osrImg = openslide_open(imgFilePath.c_str());
        RegionTemplate* inputRT = new RegionTemplate();
        inputRT->setName(IN_RT_NAME);

        // Instantiate the initial region template with the input mask image
        std::string maskFilePath = inputFolderPath + imgBasename + ".mask.tiff";
        openslide_t* osrMask = openslide_open(maskFilePath.c_str());
        RegionTemplate* maskRT = new RegionTemplate();
        maskRT->setName(MASK_RT_NAME);

        // Get input image info
        int32_t maxLevel = getLargestLevel(osrImg);
        int64_t h, w;
        openslide_get_level_dimensions(osrImg, maxLevel, &w, &h);

        int xTiles = floor(w/tSize);
        int yTiles = floor(h/tSize);

        // Create regular tiles
        // for (int ti=0; ti<yTiles; ti++) {
        //     for (int tj=0; tj<xTiles; tj++) {
        int ti=0;
        int tj=0;
                int xi = tj*tSize;
                int xo = xi+tSize-1;
                int yi = ti*tSize;
                int yo = yi+tSize-1;
                std::string tilename = "t" + std::to_string(xi) + "-" 
                    + std::to_string(xo) + "x" + std::to_string(yi) 
                    + "-" + std::to_string(yo);

                // Create tiles for each RT
                inputRT->insertDataRegion(getDDRTile(osrImg, xi, yi, 
                    xo, yo, maxLevel, tilename, REF_DDR_NAME));
                // maskRT->insertDataRegion(getDDRTile(osrMask, xi, yi, 
                //     xo, yo, 0, REF_DDR_NAME, tilename));

                // Instantiate the stages for each tile
                NormalizationComp* norm = genNormalization(inputRT, tilename);
                Segmentation* seg = genSegmentation(norm->getId(), 
                    inputRT, tilename);
                DiffMaskComp* diff = genDiffMaskComp(seg->getId(), 
                    inputRT, maskRT, REF_DDR_NAME);
                diffComponentIds.push_back(diff->getId());

                // add stages to execution
                sysEnv.executeComponent(norm);
                sysEnv.executeComponent(seg);
                // sysEnv.executeComponent(diff);
        //     }
        // }

        // // Create irregular border tiles for the last vertical column
        // if (w/tSize > xTiles) {
        //     int xi = xTiles*tSize;
        //     int xo = w-1;
        //     for (int ti=0; ti<yTiles; ti++) {
        //         int yi = ti*tSize;
        //         int yo = yi+tSize-1;
        //         std::string tilename = "<" + std::to_string(xi) + ":" 
        //             + std::to_string(xo) + "," + std::to_string(yi) 
        //             + ":" + std::to_string(yo) + ">";

        //         // Create tiles for each RT
        //         inputRT->insertDataRegion(getDDRTile(osrImg, xi, yi, 
        //             xo, yo, maxLevel, REF_DDR_NAME, tilename));
        //         // maskRT->insertDataRegion(getDDRTile(osrMask, xi, yi, 
        //         //     xo, yo, maxLevel, REF_DDR_NAME, tilename));

        //         // Instantiate the stages for each tile
        //         NormalizationComp* norm = genNormalization(inputRT, tilename);
        //         Segmentation* seg = genSegmentation(norm->getId(), 
        //             inputRT, tilename);
        //         DiffMaskComp* diff = genDiffMaskComp(seg->getId(), 
        //             inputRT, maskRT, REF_DDR_NAME);
        //         diffComponentIds.push_back(diff->getId());

        //         // add stages to execution
        //         sysEnv.executeComponent(norm);
        //         sysEnv.executeComponent(seg);
        //         // sysEnv.executeComponent(diff);
        //     }
        // }

        // // Create irregular border tiles for the last horizontal line
        // if (h/tSize > yTiles) {
        //     int yi = yTiles*tSize;
        //     int yo = h-1;
        //     for (int tj=0; tj<xTiles; tj++) {
        //         int xi = tj*tSize;
        //         int xo = xi+tSize-1;
        //         std::string tilename = "<" + std::to_string(xi) + ":" 
        //             + std::to_string(xo) + "," + std::to_string(yi) 
        //             + ":" + std::to_string(yo) + ">";

        //         // Create tiles for each RT
        //         inputRT->insertDataRegion(getDDRTile(osrImg, xi, yi, 
        //             xo, yo, maxLevel, REF_DDR_NAME, tilename));
        //         // maskRT->insertDataRegion(getDDRTile(osrMask, xi, yi, 
        //         //     xo, yo, maxLevel, REF_DDR_NAME, tilename));

        //         // Instantiate the stages for each tile
        //         NormalizationComp* norm = genNormalization(inputRT, tilename);
        //         Segmentation* seg = genSegmentation(norm->getId(), 
        //             inputRT, tilename);
        //         DiffMaskComp* diff = genDiffMaskComp(seg->getId(), 
        //             inputRT, maskRT, REF_DDR_NAME);
        //         diffComponentIds.push_back(diff->getId());

        //         // add stages to execution
        //         sysEnv.executeComponent(norm);
        //         sysEnv.executeComponent(seg);
        //         // sysEnv.executeComponent(diff);
        //     }
        // }

        // // Create irregular border tile for the bottom right last tile
        // if (w/tSize > xTiles && h/tSize > yTiles) {
        //     int xi = xTiles*tSize;
        //     int xo = w-1;
        //     int yi = yTiles*tSize;
        //     int yo = h-1;
        //     std::string tilename = "<" + std::to_string(xi) + ":" 
        //         + std::to_string(xo) + "," + std::to_string(yi) 
        //         + ":" + std::to_string(yo) + ">";

        //     // Create tiles for each RT
        //     inputRT->insertDataRegion(getDDRTile(osrImg, xi, yi, 
        //             xo, yo, maxLevel, REF_DDR_NAME, tilename));
        //     // maskRT->insertDataRegion(getDDRTile(osrMask, xi, yi, 
        //     //     xo, yo, maxLevel, REF_DDR_NAME, tilename));

        //     // Instantiate the stages for each tile
        //         NormalizationComp* norm = genNormalization(inputRT, tilename);
        //     Segmentation* seg = genSegmentation(norm->getId(), 
        //         inputRT, tilename);
        //     DiffMaskComp* diff = genDiffMaskComp(seg->getId(), 
        //         inputRT, maskRT, REF_DDR_NAME);
        //     diffComponentIds.push_back(diff->getId());

        //     // add stages to execution
        //     sysEnv.executeComponent(norm);
        //     sysEnv.executeComponent(seg);
        //     // sysEnv.executeComponent(diff);
        // }

        // int xi = 300, yi = 150;
        // int xo = 5500, yo = 4000;
        // cv::Mat tile;
        // osrRegionToCVMat(osr, xi, yi, xo, yo, maxLevel, tile);

        // cv::imwrite("tileTest.png", tile);

        // return -1;
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



