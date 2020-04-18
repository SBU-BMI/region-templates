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
    // return ExecEngineConstants::GPU;
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
        pipeline1(im_ios, target, params);
    }
} pipeline1_s;
bool r1 = RTF::AutoStage::registerStage(&pipeline1_s);

int main(int argc, char *argv[]) {
    // Manages inputs
    if (argc < 2) {
        cout << "Usage: ./iwpp <I image> [ARGS]" << endl;
        cout << "\t-c <number of cpu threads per node "
             << "(default=1)>" << endl;
        cout << "\t-g <number of gpu threads per node "
             << "(default=0)>" << endl;
        cout << "\t-t <Number of tiles per resource thread "
             << " to be generated (default=1)>" << endl;
        cout << "\t-b <tiling border (default=0)>" << endl;
        cout << "\t-p <bgThr>/<erode>/<dilate> (default=150/4/2)" << endl;
        cout << "\t-to (tiling only: generate tile images "
             << "without executing)" << endl;

        cout << "\t-pt (with pre-tiler)" << endl;
        cout << "\t-pto (with pre-tiler only, i.e., no dense)" << endl;

        cout << "\t-a <tiling algorithm>" << endl;
        cout << "\t\tValues (default=0):" << endl;
        cout << "\t\t0: No tiling (supports non-svs images)" << endl;
        cout << "\t\t1: CPU-only dense tiling" << endl;
        cout << "\t\t2: Hybrid dense tiling" << endl;
        cout << "\t\t3: Pre-tiler hybrid tiling" << endl;
        cout << "\t\t4: Res-split hybrid tiling" << endl;

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
    int nTilesPerThread = 1;
    if (findArgPos("-t", argc, argv) != -1) {
        nTilesPerThread = atoi(argv[findArgPos("-t", argc, argv)+1]);
    }

    // Pre-tiler parameters
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

    // Full tiling algorithm
    TilingAlgorithm_t tilingAlg = NO_TILING;
    if (findArgPos("-a", argc, argv) != -1) {
        tilingAlg = static_cast<TilingAlgorithm_t>(
            atoi(argv[findArgPos("-a", argc, argv)+1]));
    }

    // Dense tiling algorithm
    TilerAlg_t denseTilingAlg = FIXED_GRID_TILING;
    if (findArgPos("-d", argc, argv) != -1) {
        denseTilingAlg = static_cast<TilerAlg_t>(
            atoi(argv[findArgPos("-d", argc, argv)+1]));
    }

    int border = 0;
    if (findArgPos("-b", argc, argv) != -1) {
        border = atoi(argv[findArgPos("-b", argc, argv)+1]);
    }

    // Tiling only
    bool tilingOnly = false;
    if (findArgPos("-to", argc, argv) != -1) {
        tilingOnly = true;
    }

    bool preTile = false;
    if (findArgPos("-pt", argc, argv) != -1) {
        preTile = true;
    }

    bool preTilingOnly = false;
    if (findArgPos("-pto", argc, argv) != -1) {
        preTile = true;
        preTilingOnly = true;
    }

    float cpuPats = 1.0;
    float gpuPats = 1.7;

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
    
    // Creates the inputs using RT's autoTiler
#ifdef PROFILING
    long tilingT1 = Util::ClockGetTime();
#endif
    BGMasker* bgm = new ThresholdBGMasker(bgThr, dilate_param, erode_param);
    // CostFunction* cfunc = new PropagateDistCostFunction(bgThr, erode_param, dilate_param);
    CostFunction* cfunc = new ThresholdBGCostFunction(bgThr, dilate_param, erode_param);
    
    TiledRTCollection* tCollImg;
    switch (tilingAlg) {
        case NO_TILING:
            tCollImg = new TiledRTCollection("input", "input", Ipath, border, cfunc);
            break;
        case CPU_DENSE: {
            int nTiles = nTilesPerThread*cpuThreads;
            if (denseTilingAlg == FIXED_GRID_TILING) {
                tCollImg = new RegTiledRTCollection("input", 
                    "input", Ipath, nTiles, border, cfunc);
            } else {
                tCollImg = new IrregTiledRTCollection("input", 
                    "input", Ipath, border, cfunc, bgm, 
                    denseTilingAlg, nTiles);
            }
            break;
        }
        case HYBRID_DENSE:
            tCollImg = new HybridDenseTiledRTCollection(
                "input", "input", Ipath, border, cfunc, bgm, denseTilingAlg, 
                nTilesPerThread*cpuThreads, nTilesPerThread*gpuThreads,
                cpuPats, gpuPats);
            break;
        case HYBRID_PRETILER:
            // HybridTiledRTCollection ht = new HybridTiledRTCollection(
            //     "input", "input", Ipath, {borderL1, borderL2}, 
            //     {cfuncR1, cfuncR2}, preTilerAlg, {tilerAlgR1, tilerAlgR2}, 
            //     {nTilesR1, nTilesR2});
            break;
        case HYBRID_RESSPLIT:
            break;
    }

    BGPreTiledRTCollection preTiler("input", "input", 
            Ipath, border, cfunc, bgm);
    if (preTile) {
        cout << "[main] pre-tiling" << endl;
        preTiler.addImage(Ipath);
        preTiler.tileImages(tilingOnly);
        cout << "[main] pre-tiling done" << endl;
        tCollImg->setPreTiles(preTiler.getDense());
        if (preTilingOnly) {
            preTiler.generateDRs(tilingOnly);
            cout << "[main] pre-tiles generated" << endl;
            tCollImg = &preTiler;
        }
    }

    if (!preTilingOnly) {
        tCollImg->addImage(Ipath);
        tCollImg->tileImages(tilingOnly);
        if (preTile)
            tCollImg->addTiles(preTiler.getBg());
        tCollImg->generateDRs(tilingOnly);
    }

#ifdef PROFILING
    long tilingT2 = Util::ClockGetTime();
    cout << "[PROFILING][TILING_TIME] " << (tilingT2-tilingT1) << endl;
    cout << "[PROFILING][TILES] " << tCollImg->getNumRTs() << endl;
#endif

    if (tilingOnly) {
        sysEnv.startupExecution();
        sysEnv.finalizeSystem();
        return 0;
    }

    // Create an instance of the two stages for each image tile pair
    // and also send them for execution
    std::vector<cv::Rect_<int>> tiles;
    std::list<cv::Rect_<int64_t>> l = tCollImg->getTiles()[0];
    for (cv::Rect_<int64_t> tile : l) {
        // std::cout << tile.x << ":" << tile.width << "," 
        //     << tile.y << ":" << tile.height << std::endl;
        tiles.emplace_back(tile);
    }

    RegionTemplate* rtOut = newRT("rtOut");

    for (int i=0; i<tCollImg->getNumRTs(); i++) {
        RTF::AutoStage stage0({tCollImg->getRT(i).second, rtOut}, 
            {new ArgumentInt(i), 
             new ArgumentInt(blue), new ArgumentInt(green), new ArgumentInt(red), 
             new ArgumentInt(0),
             new ArgumentInt(disk19raw_width), new ArgumentIntArray(disk19raw, disk19raw_size),
             new ArgumentInt(G1),
             new ArgumentInt(se3raw_width), new ArgumentIntArray(se3raw, se3raw_size)}, 
            {tiles[i].height, tiles[i].width}, {&pipeline1_s}, 
            tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        stage0.genStage(sysEnv);

        // RTF::AutoStage stage5({rtRC, rtRcOpen, rtRecon}, 
        //     {new ArgumentInt(0), new ArgumentInt(0), 
        //      new ArgumentInt(i), new ArgumentInt(iwppOp)}, 
        //     {tiles[i].height, tiles[i].width}, {&imreconstruct}, 
        //     tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        // stage5.after(&stage4);
        // stage5.genStage(sysEnv);
    }

    sysEnv.startupExecution();
    sysEnv.finalizeSystem();

#ifdef PROFILING
    long fullExecT2 = Util::ClockGetTime();
    cout << "[PROFILING][FULL_TIME] " << (fullExecT2-fullExecT1) << endl;
#endif

    return 0;
}
