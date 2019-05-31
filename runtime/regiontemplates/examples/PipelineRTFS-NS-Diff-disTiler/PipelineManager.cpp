#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <math.h>
#include <unistd.h> // Used for args/flags parsing

#include "SysEnv.h"
#include "RegionTemplate.h"
#include "BoundingBox.h"
#include "Util.h"

#include "NormalizationComp.h"
#include "Segmentation.h"
#include "DiffMaskComp.h"

#include "RTCollectionTilingPipeline.h"
#include "TilingStage.h"
#include "RegularTilingStage.h"
#include "ThresholdBGRemStage.h"
#include "LogSweepTilingStage.h"

// #include "TiledRTCollection.h"
// #include "RegTiledRTCollection.h"
// #include "IrregTiledRTCollection.h"
// #include "costFuncs/BGMasker.h"
// #include "costFuncs/ThresholdBGMasker.h"

static const std::string IN_RT_NAME = "img";
static const std::string MASK_RT_NAME = "mask";
// static const std::string REF_DDR_NAME = "initial";

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
    RegionTemplate* rtMask, std::string inDdrName, std::string maskDdrName) {

    DiffMaskComp* diff = new DiffMaskComp();

    diff->setIo(rtIn->getName(), rtMask->getName(), inDdrName, maskDdrName);

    diff->addArgument(new ArgumentString(rtIn->getName()));
    diff->addArgument(new ArgumentString(rtMask->getName()));
    diff->addArgument(new ArgumentString(inDdrName));
    diff->addArgument(new ArgumentString(maskDdrName));

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

// Possible stages of the tiling pipeline
// Have their arg prefix commented by its size
enum stages_t {
    REG_TILING, // REG
    THR_BG_REM, // TBR
    LOG_SWEEP,  // LGS
    KD_TREE,    // KDT
};

int str2stages_t (char* name) {
    std::string s(name);
    if (s.compare("reg")) return REG_TILING;
    else if (s.compare("tbr")) return THR_BG_REM;
    else if (s.compare("lgs")) return LOG_SWEEP;
    else if (s.compare("kdt")) return KD_TREE;
    else return -1;
}

// Enum string values of stages_t
const char* stages_t2str[] = {
    "reg", // REG_TILING
    "tbr", // THR_BG_REM
    "lgs", // LOG_SWEEP
    "kdt", // KD_TREE
};

enum stageArgs_t {
    ALL_IN_IMG,      // i: Input image path
    // b: Overlap border (for all tiles at the end of the pipeline)
    ALL_OVLP_BORDER,

    REG_T_SIZE,      // t: Tile size
    
    TBR_THRV,        // v: Threshold value
    TBR_THRO,        // o: Threshold orientation (greater or less than val)
    TBR_DIL,         // d: Dilation/erosion size

    LGS_NTILES,      // n: Max number of tiles (XOR with maxc)
    LGS_MAXC,        // c: Max cost of a tile (XOR with ntiles)
    LGS_VAR,         // m: Margin of variance for max cost of tiles
};

void printHelp() {
    std::cout << "Tiling pipeline example for segm workflow" << std::endl;
    std::cout << "Usage: ./PipelineRTFS-NS-Diff-disTiler " 
        << "[options]... stages..." << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "-i   Input images path. Formated as 'path/<filename>' on " 
        << " which there are two files on path: <filename>.svs (the input "
        << "image) and <filename>.tiff (the mask image)." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "" << std::endl;
}

// Returns a map of all found arguments' values by their arg id of stageArgs_t
std::map<stageArgs_t, std::string> loadArgs(int argc, char **argv) {
    // Output map of args
    std::map<stageArgs_t, std::string> args;

    // "i"   = i is a bool value
    // "i:"  = i requires an argument
    // "i::" = i may have an argument
    std::string argOpts = "i:t:v:o:d:n:c:m:";

    int c;
    while ((c = getopt (argc, argv, argOpts.c_str())) != -1) {
        switch (c) {
            // General args
            case 'i': args[ALL_IN_IMG] = optarg; break;
            case 'b': args[ALL_OVLP_BORDER] = optarg; break;

            // Regular tiling args
            case 't': args[REG_T_SIZE] = optarg; break;

            // Threshold background removal
            case 'v': args[TBR_THRV] = optarg; break;
            case 'o': args[TBR_THRO] = optarg; break;
            case 'd': args[TBR_DIL] = optarg; break;

            // Log-sweeping tiling
            case 'n': args[LGS_NTILES] = optarg; break;
            case 'c': args[LGS_MAXC] = optarg; break;
            case 'm': args[LGS_VAR] = optarg; break;

            default: {
                std::cout << "Bad args." << std::endl;
                exit(-1);
            }
        }
    }

    return args;
}

// Gets all pipeline stages from argv and instantiates them into the pipeline
// with the parsed parameters
void generateTilingPipeline(RTCollectionTilingPipeline* pipeline, 
    int argc, char** argv, const std::map<stageArgs_t, std::string>& args) {

    TilingStage* stage;
    for (int i = optind; i<argc; i++) {
        switch (str2stages_t(argv[i])) {
            case REG_TILING: {
                stage = new RegularTilingStage();
                if (args.find(REG_T_SIZE) != args.end())
                    ((RegularTilingStage*)stage)->setTileSize(args.at(REG_T_SIZE));
                break;
            }
            case THR_BG_REM: {
                stage = new ThresholdBGRemStage();
                if (args.find(TBR_THRV) != args.end())
                    ((ThresholdBGRemStage*)stage)->setThrhd(args.at(TBR_THRV));
                if (args.find(TBR_THRO) != args.end())
                    ((ThresholdBGRemStage*)stage)->setOrient(args.at(TBR_THRO));
                if (args.find(TBR_DIL) != args.end())
                    ((ThresholdBGRemStage*)stage)->setErode(args.at(TBR_DIL));
                break;
            }
            case LOG_SWEEP: {
                stage = new LogSweepTilingStage();
                if (args.find(LGS_NTILES) != args.end())
                    ((LogSweepTilingStage*)stage)->setNTiles(args.at(LGS_NTILES));
                if (args.find(LGS_MAXC) != args.end())
                    ((LogSweepTilingStage*)stage)->setMaxC(args.at(LGS_MAXC));
                if (args.find(LGS_VAR) != args.end())
                    ((LogSweepTilingStage*)stage)->setVar(args.at(LGS_VAR));
                break;
            }
            case KD_TREE: {
                // stage = new ThresholdBGRemStage();
                // if (args.find(TBR_THRV) != args.end())
                //     ((RegularTilingStage*)stage)->setThrhd(args.at(TBR_THRV]);
                break;
            }
            default: {
                std::cout << "Unknown stage: " << argv[i] << std::endl;
                exit(-1);
            }
        }
        pipeline->addStage(stage);
    }
}

int main(int argc, char **argv){
    // Current version only reads a single image as a flag and many
    // arguments containing the tiling pipeline
    std::map<stageArgs_t, std::string> args = loadArgs(argc, argv);
    std::string inputImgPath = args.at(ALL_IN_IMG);

    // Instantiates a new RT collection tiling pipeline for the initial image
    RTCollectionTilingPipeline* inTilingPipeline 
        = new RTCollectionTilingPipeline(IN_RT_NAME);
    inTilingPipeline->setImage(inputImgPath + ".svs");
    if (args.find(ALL_OVLP_BORDER) != args.end())
        inTilingPipeline->setBorder(atoi(args.at(ALL_OVLP_BORDER).c_str()));

    // Sets profiling info
    // inTilingPipeline->setProfiling(FULL);

    // Adds the tiling stages to the pipeline
    generateTilingPipeline(inTilingPipeline, argc, argv, args);

    // Executes tiling pipeline
    inTilingPipeline->tile();

    // Applies the same tiling to the mask image
    RTCollectionTilingPipeline* maskTilingPipeline 
        = new RTCollectionTilingPipeline(MASK_RT_NAME);
    bool noDiff = false;
    if (!noDiff) { // Only needs mask if the is one
        maskTilingPipeline->setImage(inputImgPath + ".tiff");
        maskTilingPipeline->tile(inTilingPipeline);
    }

    // Handler to the distributed execution system environment
    SysEnv sysEnv;

    // Tells the system which libraries should be used
    sysEnv.startupSystem(argc, argv, "libcomponentnsdiffdt.so");

    // Generate the pipelines for each tile
    std::vector<int> diffComponentIds; // Diff result vector
    for (int i=0; i<inTilingPipeline->getNumRTs(); i++) {
        // Get RTs
        std::string inputRTDRname = inTilingPipeline->getRT(i).first;
        RegionTemplate* inputRT = inTilingPipeline->getRT(i).second;

        // Instantiate stages
        NormalizationComp* norm = genNormalization(inputRT, inputRTDRname);
        Segmentation* seg = genSegmentation(norm->getId(), 
            inputRT, inputRTDRname);

        // add stages to execution
        sysEnv.executeComponent(norm);
        sysEnv.executeComponent(seg);

        // Add the diff stage if required
        // Diff can be disabled if one wants to get the mask results
        if (!noDiff) {
            std::string maskRTDRname = maskTilingPipeline->getRT(i).first;
            RegionTemplate* maskRT = maskTilingPipeline->getRT(i).second;

            DiffMaskComp* diff = genDiffMaskComp(seg->getId(), 
                inputRT, maskRT, inputRTDRname, maskRTDRname);
            diffComponentIds.push_back(diff->getId());
            
            sysEnv.executeComponent(diff);
        }
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







//     // get args
//     if (argc < 5) {
//         std::cout << "Usage: ./exec [-t <sqr_dim>] -i <input_basename> "
//             << "-m <mask_basename> -p <path>" << std::endl;
//         std::cout << "\t-t: Break images into sqr_dim sized squares for "
//             << "execution, i.e., don't use disTiler. 0 for no tiling. " 
//             << "For irregular tiling omit this parameter."
//             << std::endl;
//         std::cout << "\t-i: Full name of the input image. "
//             << "It MUST be a .svs file." << std::endl;
//         std::cout << "\t-m: Full name of the mask image. "
//             << "It can be a .svs file." << std::endl;
//         std::cout << "\t-p: Path of all input images" << std::endl;
//         std::cout << "\t-l: Whether input tiles should be generated lazily. " 
//             << "Only applicable for irregular tiling." << std::endl;
//         std::cout << "\t-nt: Number of expected dense tiles. " 
//             << "Only applicable for irregular tiling." << std::endl;
//         exit(0);
//     }

//     // Regular tiling?
//     int tSize = -1;
//     if (findArgPos("-t", argc, argv) != -1) {
//         tSize = atoi(argv[findArgPos("-t", argc, argv)+1]);
//     }

//     // Input images
//     string imgBasename;
//     if (findArgPos("-i", argc, argv) == -1) {
//         cout << "Missing image basename." << endl;
//         return 0;
//     } else {
//         imgBasename = argv[findArgPos("-i", argc, argv)+1];
//         std::size_t l = imgBasename.find_last_of(".");
//         if (imgBasename.substr(l).compare(".svs") != 0) {
//             cout << "Input image not svs." << endl;
//             return 0;
//         }
//     }

//     // Mask images
//     string maskBasename;
//     if (findArgPos("-m", argc, argv) == -1) {
//         cout << "Missing mask basename." << endl;
//         return 0;
//     } else {
//         maskBasename = argv[findArgPos("-m", argc, argv)+1];
//         std::size_t l = maskBasename.find_last_of(".");
//         // if (maskBasename.substr(l).compare(".svs") != 0) {
//         //     cout << "Mask image not svs." << endl;
//         //     return 0;
//         // }
//     }

//     // Input images
//     string inputFolderPath;
//     if (findArgPos("-p", argc, argv) == -1) {
//         cout << "Missing input images path." << endl;
//         return 0;
//     } else {
//         inputFolderPath = argv[findArgPos("-p", argc, argv)+1];
//     }

//     // Lazy tiling?
//     bool lazyTileRead = false;
//     if (findArgPos("-l", argc, argv) != -1) {
//         lazyTileRead = true;
//     }

//     // Number of expected dense tiles for irregular tiling
//     int nTiles = 0;
//     if (findArgPos("-nt", argc, argv) != -1) {
//         nTiles = atoi(argv[findArgPos("-nt", argc, argv)+1]);
//     }











//     // For this test we only use a single input and output images
//     std::string imgFilePath = inputFolderPath + imgBasename;
//     std::string maskFilePath = inputFolderPath + maskBasename;
    
//     TiledRTCollection* tCollImg;
//     TiledRTCollection* tCollMask;
//     std::string tmpPath = "./";
//     std::vector<int> diffComponentIds;
//     int border = 0; // pixels to be added to the borders of the tiles
        
//     // Tiling profile time
//     uint64_t initTime = Util::ClockGetTime();

//     if (tSize == 0) { // no tiling
//         // Generate TRTCs for the standard tiling i.e., no tiling at all
//         tCollImg = new TiledRTCollection(IN_RT_NAME, REF_DDR_NAME, tmpPath);
//         tCollMask = new TiledRTCollection(MASK_RT_NAME, REF_DDR_NAME, tmpPath);
//     } else if (tSize > 0) { // regular tiling
//         // Generate TRTCs for tiling with square tiles of tSize x tSize
//         tCollImg = new RegTiledRTCollection(IN_RT_NAME, 
//             REF_DDR_NAME, tmpPath, tSize, tSize, border);
//         tCollMask = new RegTiledRTCollection(MASK_RT_NAME, 
//             REF_DDR_NAME, tmpPath, tSize, tSize, border);
//     } else if (tSize < 0) { // irregular area autotiler 
//         int bgThr = 100;
//         int dilate = 10;
//         BGMasker* bgm = new ThresholdBGMasker(bgThr, dilate);

//         // Masking test
//         // cv::Mat mask = bgm->bgMask(cv::imread(imgFilePath));
//         // cv::imwrite("./testmask.png", mask);
//         // exit(9);

//         if (lazyTileRead) {
//             tCollImg = new IrregTiledRTCollection(IN_RT_NAME, 
//                 REF_DDR_NAME, imgFilePath, border, bgm, nTiles);
//             ((IrregTiledRTCollection*)tCollImg)->setLazyReading();
//         } else {
//             tCollImg = new IrregTiledRTCollection(IN_RT_NAME, 
//                 REF_DDR_NAME, tmpPath, border, bgm, nTiles);
//         }
//         tCollMask = new IrregTiledRTCollection(MASK_RT_NAME, 
//             REF_DDR_NAME, tmpPath, border, bgm);
//     }

//     // Add the images to be tiled
//     tCollImg->addImage(imgFilePath);
//     tCollMask->addImage(maskFilePath);

//     // Perform standard tiling i.e., no tiling at all
//     tCollImg->tileImages();
// #ifndef NODIFF
//     tCollMask->tileImages(tCollImg->getTiles());
// #endif

//     uint64_t endTime = Util::ClockGetTime();
//     std::string tilingType = tSize < 0 ? "Irregular" : "Regular";
//     std::string laziness = tSize < 0 && lazyTileRead ? " lazy" : "";
//     std::string rtSize = tSize >= 0 ? " with tile size " + to_string(tSize) : "";
//     std::cout << "[PielineManager] " << tilingType << laziness << " tiling" 
//         << rtSize <<  " in " << ((float)(endTime-initTime)/1000) 
//         << " seconds " << endTime << " -> " << endTime << std::endl;

//     std::cout << "[PielineManager] Number of tiles created: " 
//         << tCollImg->getNumRTs() << std::endl;







//     // Generate the pipelines for each tile
//     for (int i=0; i<tCollImg->getNumRTs(); i++) {
//         // Get RTs
//         std::string inputRTDRname = tCollImg->getRT(i).first;
//         RegionTemplate* inputRT = tCollImg->getRT(i).second;
// #ifndef NODIFF
//         std::string maskRTDRname = tCollMask->getRT(i).first;
//         RegionTemplate* maskRT = tCollMask->getRT(i).second;
// #endif

//         // Instantiate stages
//         NormalizationComp* norm = genNormalization(inputRT, inputRTDRname);
//         Segmentation* seg = genSegmentation(norm->getId(), 
//             inputRT, inputRTDRname);
// #ifndef NODIFF
//         DiffMaskComp* diff = genDiffMaskComp(seg->getId(), 
//             inputRT, maskRT, inputRTDRname, maskRTDRname);
//         diffComponentIds.push_back(diff->getId());
// #endif

//         // add stages to execution
//         sysEnv.executeComponent(norm);
//         sysEnv.executeComponent(seg);
// #ifndef NODIFF
//         sysEnv.executeComponent(diff);
// #endif
//     }

//     // End Creating Dependency Graph
//     sysEnv.startupExecution();

//     float diff_f = 0;
//     float secondaryMetric = 0;
//     for(int i = 0; i < diffComponentIds.size(); i++){
//         char *resultData = sysEnv.getComponentResultData(diffComponentIds[i]);
//         std::cout << "Diff Id: " << diffComponentIds[i] << " resultData -  ";
//         if(resultData != NULL){
//             std::cout << "size: " << ((int *) resultData)[0] 
//                 << " Diff: " << ((float *) resultData)[1] 
//                 << " Secondary Metric: " << ((float *) resultData)[2] 
//                 << std::endl;
//             diff_f += ((float *) resultData)[1];
//             secondaryMetric += ((float *) resultData)[2];
//         }else{
//             std::cout << "NULL" << std::endl;
//         }
//     }
//     std::cout << "Total diff: " << diff_f << " Secondary Metric: " 
//         << secondaryMetric << std::endl;

//     // std::string tilingType = tSize < 0 ? "Irregular" : "Regular";
//     // std::string laziness = tSize < 0 && lazyTileRead ? " lazy" : "";
//     // std::string rtSize = 
//     //     tSize >= 0 ? " with tile size " + to_string(tSize) : "";
//     // std::cout << "[PielineManager] " << tilingType << laziness << " tiling" 
//     //     << rtSize <<  " in " << ((float)(endTime-initTime)/1000) 
//     //     << " seconds " << endTime << " -> " << endTime << std::endl;

//     // Finalize all processes running and end execution
//     sysEnv.finalizeSystem();

//     return 0;
}



