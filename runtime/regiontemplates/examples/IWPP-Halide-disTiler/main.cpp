#include <iostream>
#include <stdio.h>

#include "cv.hpp"
#include "Halide.h"

#include "AutoStage.h"
#include "IwppRecon.h"
#include "RegionTemplate.h"
#include "ExecEngineConstants.h"
#include "Util.h"

#include "TiledRTCollection.h"
#include "RegTiledRTCollection.h"
#include "IrregTiledRTCollection.h"
#include "HybridDenseTiledRTCollection.h"
#include "BGPreTiledRTCollection.h"
#include "costFuncs/BGMasker.h"

#include "CostFunction.h"
#include "costFuncs/ThresholdBGMasker.h"
#include "costFuncs/ColorThresholdBGMasker.h"
#include "costFuncs/ThresholdBGCostFunction.h"
#include "costFuncs/OracleCostFunction.h"
#include "costFuncs/PropagateDistCostFunction.h"
#include "costFuncs/MultiObjCostFunction.h"
#include "costFuncs/AreaCostFunction.h"

#include "HalideIwpp.h"
#include "pipeline1.h"

using std::cout;
using std::endl;

#define PROFILING
#define PROFILING_STAGES

enum TilingAlgorithm_t {
    NO_TILING,
    CPU_DENSE,
    HYBRID_DENSE,
    HYBRID_PRETILER,
    HYBRID_RESSPLIT,
};

enum CostFunction_t {
    THRS,
    MV_THRS_AREA,
};

// Should use ExecEngineConstants::GPU ... 
typedef int Target_t;
inline Target_t tgt(TilingAlgorithm_t tilingAlg, Target_t target) {
    switch (tilingAlg) {
        case NO_TILING:
        case CPU_DENSE:
            return ExecEngineConstants::CPU;
        case HYBRID_DENSE:
        case HYBRID_PRETILER:
        case HYBRID_RESSPLIT:
            return target;
    }
    // return ExecEngineConstants::ANY;
}

int findArgPos(std::string s, int argc, char** argv) {
    for (int i=1; i<argc; i++)
        if (std::string(argv[i]).compare(s)==0)
            return i;
    return -1;
}

static struct : RTF::HalGen {
    std::string getName() {return "pipeline1";}
    bool realize(std::vector<cv::Mat>& im_ios, Target_t target, 
                 std::vector<ArgumentBase*>& params) {
        std::cout << "[pipeline1] executing pipeline" << std::endl;
        pipeline1(im_ios, target, params);
    }
} pipeline1_s;
bool r1 = RTF::AutoStage::registerStage(&pipeline1_s);

int main(int argc, char *argv[]) {
    // Manages inputs
    if (argc < 2) {
        cout << "Usage: ./iwpp <I image> [ARGS]" << endl;
        cout << "Executing tiling pipeline PT -> EL-TH on dense, EL-A-half "
             << "or EL-A-fold on BG" << endl;
        cout << "Tiles sorted as: Dense, BG" << endl << endl;

        cout << "=== General options:" << endl;
        cout << "\t-c <number of cpu threads per node "
             << "(default=1)>" << endl;
        cout << "\t-g <number of gpu threads per node "
             << "(default=0)>" << endl;
        cout << "\t-t <Number of tiles to be generated (default=1)>" << endl;
        cout << "\t-b <tiling border (default=0)>" << endl;
        cout << "\t-p <bgThr>/<erode>/<dilate> (default=150/4/2)" << endl;
        cout << "\t-to (tiling only: generate tile images "
             << "without executing)" << endl;
        cout << "\t-nt (execute full image without tiling, overriding "
             << "any other tiling parameter)" << endl;

        cout << "=== Pre-tiler (PT) options:" << endl;
        cout << "\t-npt (without pre-tiler)" << endl;
        // cout << "\t-pto (with pre-tiler only, i.e., no dense)" << endl;

        // cout << "\t-a <tiling algorithm>" << endl;
        // cout << "\t\tValues (default=0):" << endl;
        // cout << "\t\t0: No tiling (supports non-svs images)" << endl;
        // cout << "\t\t1: CPU-only dense tiling" << endl;
        // cout << "\t\t2: Hybrid dense tiling" << endl;
        // cout << "\t\t3: Pre-tiler hybrid tiling" << endl;
        // cout << "\t\t4: Res-split hybrid tiling" << endl;
        
        cout << "=== Dense tiling options:" << endl;
        cout << "\t-d <dense tiling algorithm>" << endl;
        cout << "\t\tValues (default=0):" << endl;
        cout << "\t\t0: FIXED_GRID_TILING" << endl;
        cout << "\t\t1: LIST_ALG_HALF" << endl;
        cout << "\t\t2: LIST_ALG_EXPECT" << endl;
        cout << "\t\t3: KD_TREE_ALG_AREA" << endl;
        cout << "\t\t4: KD_TREE_ALG_COST" << endl;
        cout << "\t\t5: HBAL_TRIE_QUAD_TREE_ALG" << endl;
        cout << "\t\t6: CBAL_TRIE_QUAD_TREE_ALG" << endl;
        cout << "\t\t7: CBAL_POINT_QUAD_TREE_ALG" << endl;

        cout << "\t-f <dense cost function>" << endl;
        cout << "\t\tValues (default=0):" << endl;
        cout << "\t\t0: THRS" << endl;
        cout << "\t\t1: MV_THRS_AREA" << endl;

        cout << "\t-m <execBias>/<loadBias> (default=1/100)" << endl;

        cout << "=== Background tiling (BG) options:" << endl;
        cout << "\t-k <background tiling algorithm>" << endl;
        cout << "\t\tValues (default=2):" << endl;
        cout << "\t\t2: LIST_ALG_EXPECT" << endl;
        cout << "\t\t1: LIST_ALG_HALF" << endl;
        
        // cout << "\t-xb (sort background tiles with area cost function)" << endl;

        // cout << "\t-tb (tile background tiles with area cost function)" << endl;
        
        exit(0);
    }

    // Input images
    std::string Ipath = std::string(argv[1]);

    // Number of cpu threads
    int cpuThreads = 1;
    if (findArgPos("-c", argc, argv) != -1) {
        cpuThreads = atoi(argv[findArgPos("-c", argc, argv)+1]);
    }

    // Number of gpu threads
    int gpuThreads = 1;
    if (findArgPos("-g", argc, argv) != -1) {
        gpuThreads = atoi(argv[findArgPos("-g", argc, argv)+1]);
    }

    // Number of expected dense tiles for irregular tiling
    int nTiles = 1;
    if (findArgPos("-t", argc, argv) != -1) {
        nTiles = atoi(argv[findArgPos("-t", argc, argv)+1]);
    }

    int border = 0;
    if (findArgPos("-b", argc, argv) != -1) {
        border = atoi(argv[findArgPos("-b", argc, argv)+1]);
    }

    // Threshold cost function parameters
    int bgThr = 150;
    int erode_param = 4;
    int dilate_param = 2;
    if (findArgPos("-p", argc, argv) != -1) {
        std::string params = argv[findArgPos("-p", argc, argv)+1];
        std::size_t l = params.find_last_of("/");
        dilate_param = atoi(params.substr(l+1).c_str());
        params = params.substr(0, l);
        l = params.find_last_of("/");
        erode_param = atoi(params.substr(l+1).c_str());
        bgThr = atoi(params.substr(0, l).c_str());
    }

    // Tiling only
    bool tilingOnly = false;
    if (findArgPos("-to", argc, argv) != -1) {
        tilingOnly = true;
    }

    // No tiling execution
    bool noTiling = false;
    if (findArgPos("-nt", argc, argv) != -1) {
        noTiling = true;
    }

    // No pre-tiling execution
    bool preTiling = true;
    if (findArgPos("-npt", argc, argv) != -1) {
        preTiling = false;
    }

    // Dense tiling algorithm
    TilerAlg_t denseTilingAlg = FIXED_GRID_TILING;
    if (findArgPos("-d", argc, argv) != -1) {
        denseTilingAlg = static_cast<TilerAlg_t>(
            atoi(argv[findArgPos("-d", argc, argv)+1]));
    }

    // Dense tiling algorithm
    CostFunction_t denseCostf = THRS;
    if (findArgPos("-f", argc, argv) != -1) {
        denseCostf = static_cast<CostFunction_t>(
            atoi(argv[findArgPos("-f", argc, argv)+1]));
    }

    // Threshold cost function parameters
    int execBias = 1;
    int loadBias = 100;
    if (findArgPos("-m", argc, argv) != -1) {
        std::string params = argv[findArgPos("-m", argc, argv)+1];
        std::size_t l = params.find_last_of("/");
        execBias = atoi(params.substr(0, l).c_str());
        loadBias = atoi(params.substr(l+1).c_str());
    }

    // BG tiling algorithm
    TilerAlg_t bgTilingAlg = LIST_ALG_EXPECT;
    if (findArgPos("-k", argc, argv) != -1) {
        bgTilingAlg = static_cast<TilerAlg_t>(
            atoi(argv[findArgPos("-k", argc, argv)+1]));
    }

    if (noTiling && tilingOnly) {
        cout << "Options 'tiling-only' and 'no tiling' "
             << "are mutually exclusive" << endl;
        exit(0);
    }

#ifdef PROFILING
    long fullExecT1 = Util::ClockGetTime();
#endif

    SysEnv sysEnv;
    sysEnv.startupSystem(argc, argv, "libautostage.so");

    // Input parameters
    unsigned char blue = 200;
    unsigned char green = 200;
    unsigned char red = 200;
    double T1 = 1.0;
    double T2 = 2.0;
    unsigned char G1 = 50;
    unsigned char G2 = 100;
    int minSize = 10;
    int maxSize = 100;
    int fillHolesConnectivity = 4;
    int reconConnectivity = 4;
    // 19x19
    int disk19raw_width = 19;
    int disk19raw_size = disk19raw_width*disk19raw_width;
    int disk19raw[disk19raw_size] = {
        0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
        0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
        0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
        0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
        0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0};
    
    // 3x3
    int se3raw_width = 3;
    int se3raw_size = se3raw_width*se3raw_width;
    int se3raw[se3raw_size] = {
        1, 1, 1,
        1, 1, 1, 
        1, 1, 1};
    
    #ifdef PROFILING
    long tilingT1 = Util::ClockGetTime();
    #endif

    // Tilers
    TiledRTCollection* denseTiler;
    TiledRTCollection* bgTiler;

    // Tiling cost functions
    BGMasker* bgm = new ThresholdBGMasker(bgThr, dilate_param, erode_param);
    CostFunction* denseCostFunc;
    if (denseCostf == THRS)
        denseCostFunc = new ThresholdBGCostFunction(
            static_cast<ThresholdBGMasker*>(bgm));
    else
        denseCostFunc = new MultiObjCostFunction(
            static_cast<ThresholdBGMasker*>(bgm), execBias, loadBias);
    CostFunction* bgCostFunc = new AreaCostFunction();

    // Creates dense tiling collection
    if (noTiling)
        denseTiler = new TiledRTCollection("input", 
            "input", Ipath, border, denseCostFunc);
    else if (denseTilingAlg == FIXED_GRID_TILING)
        denseTiler = new RegTiledRTCollection("input", 
            "input", Ipath, nTiles, border, denseCostFunc);
    else
        denseTiler = new IrregTiledRTCollection("input", 
            "input", Ipath, border, denseCostFunc, bgm, 
            denseTilingAlg, nTiles);

    // Performs pre-tiling, if required
    BGPreTiledRTCollection preTiler("input", "input", 
        Ipath, border, denseCostFunc, bgm);
    if (preTiling && !noTiling) {
        // Performs actual tiling
        preTiler.addImage(Ipath);
        preTiler.tileImages(tilingOnly);

        // Send outputs to dense tiling
        denseTiler->setPreTiles(preTiler.getDense());
    }

    // Performs dense tiling
    denseTiler->addImage(Ipath);
    denseTiler->tileImages(tilingOnly);

    // BG tiling
    if (preTiling && !noTiling) {
        // Calculates the number of expected tiles as a multiple of nTiles
        int bgTilesExpected = (std::floor(preTiler.getBgSize()/nTiles)+1)*nTiles;
        bgTiler = new IrregTiledRTCollection("input", "input", Ipath, 
            border, bgCostFunc, bgm, bgTilingAlg, bgTilesExpected);

        bgTiler->setPreTiles(preTiler.getBg());

        // Performs BG tiling and adds results to dense
        bgTiler->addImage(Ipath);
        bgTiler->tileImages(tilingOnly);
        denseTiler->addTiles(bgTiler->getTilesBase());
    }

    // Generates tiles and converts it to vector
    denseTiler->generateDRs(tilingOnly);
    std::vector<cv::Rect_<int>> tiles;
    std::list<cv::Rect_<int64_t>> l = denseTiler->getTiles()[0];
    cout << "[main] Tiles generated: " << l.size() << endl;
    for (cv::Rect_<int64_t> tile : l) {
        // std::cout << tile.x << ":" << tile.width << "," 
        //     << tile.y << ":" << tile.height << std::endl;
        tiles.emplace_back(tile);
    }

    #ifdef PROFILING
    long tilingT2 = Util::ClockGetTime();
    #endif

    // Create an instance of the two stages for each image tile pair
    // and also send them for execution
    RegionTemplate* rtOut = newRT("rtOut");
    for (int i=0; i<denseTiler->getNumRTs(); i++) {
        RTF::AutoStage stage0({denseTiler->getRT(i).second, rtOut}, 
            {new ArgumentInt(i), 
             new ArgumentInt(blue), new ArgumentInt(green), new ArgumentInt(red), 
             new ArgumentInt(0),
             new ArgumentInt(disk19raw_width), new ArgumentIntArray(disk19raw, disk19raw_size),
             new ArgumentInt(G1),
             new ArgumentInt(se3raw_width), new ArgumentIntArray(se3raw, se3raw_size)}, 
            {tiles[i].height, tiles[i].width}, {&pipeline1_s}, 
            denseTiler->getTileTarget(i), i);
            // tgt(tilingAlg, denseTiler->getTileTarget(i)), i);
        stage0.genStage(sysEnv);

        // RTF::AutoStage stage5({rtRC, rtRcOpen, rtRecon}, 
        //     {new ArgumentInt(0), new ArgumentInt(0), 
        //      new ArgumentInt(i), new ArgumentInt(iwppOp)}, 
        //     {tiles[i].height, tiles[i].width}, {&imreconstruct}, 
        //     tgt(tilingAlg, denseTiler->getTileTarget(i)), i);
        // stage5.after(&stage4);
        // stage5.genStage(sysEnv);
    }

    sysEnv.startupExecution();
    sysEnv.finalizeSystem();

    #ifdef PROFILING
    long fullExecT2 = Util::ClockGetTime();
    cout << "[PROFILING][TILES] " << tiles.size() << endl;
    cout << "[PROFILING][TILING_TIME] " << (tilingT2-tilingT1) << endl;
    cout << "[PROFILING][FULL_TIME] " << (fullExecT2-fullExecT1) << endl;
    #endif

    return 0;
}
