#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "Halide.h"
#include "opencv/cv.hpp"

#include "AutoStage.h"
#include "BGPreTiledRTCollection.h"
#include "CostFunction.h"
#include "ExecEngineConstants.h"
#include "Halide.h"
#include "HalideIwpp.h"
#include "HybridDenseTiledRTCollection.h"
#include "IrregTiledRTCollection.h"
#include "RegTiledRTCollection.h"
#include "RegionTemplate.h"
#include "SingleTileRTCollection.h"
#include "TiledRTCollection.h"
#include "Util.h"
#include "costFuncs/AreaCostFunction.h"
#include "costFuncs/BGMasker.h"
#include "costFuncs/ColorThresholdBGMasker.h"
#include "costFuncs/LGBCostFunction.h"
#include "costFuncs/MultiObjCostFunction.h"
#include "costFuncs/OracleCostFunction.h"
#include "costFuncs/PrePartThresBGCostFunction.h"
#include "costFuncs/PropagateDistCostFunction.h"
#include "costFuncs/ThresholdBGCostFunction.h"
#include "costFuncs/ThresholdBGMasker.h"
#include "pipeline1.h"

using std::cout;
using std::endl;

#define PROFILING
#define PROFILING_STAGES

enum HybridExec_t {
    CPU_ONLY,
    GPU_ONLY,
    HYBRID,
};

enum CostFunction_t {
    THRS,
    MV_THRS_AREA,
    TILED_THRS,
    LGB_MODEL,
};

// Should use ExecEngineConstants::GPU ...
typedef int Target_t;

inline Target_t tgt(HybridExec_t exec, Target_t target) {
    switch (exec) {
        case CPU_ONLY:
            return ExecEngineConstants::CPU;
        case GPU_ONLY:
            return ExecEngineConstants::GPU;
        case HYBRID:
            return target;
    }
    return target;
}

inline Target_t tgt_merge(HybridExec_t exec) {
    switch (exec) {
        case CPU_ONLY:
            return ExecEngineConstants::CPU;
        case GPU_ONLY:
            return ExecEngineConstants::GPU;
        case HYBRID:
            return ExecEngineConstants::CPU;
    }
}

int findArgPos(std::string s, int argc, char **argv) {
    for (int i = 1; i < argc; i++)
        if (std::string(argv[i]).compare(s) == 0)
            return i;
    return -1;
}

static struct : RTF::HalGen {
    std::string getName() { return "pipeline1"; }
    bool        realize(std::vector<cv::Mat> &im_ios, Target_t target,
                        std::vector<ArgumentBase *> &params) {
        return pipeline1(im_ios, target, params);
        // pipeline1_nscale(im_ios, target, params);
    }
} pipeline1_s;
bool r1 = RTF::AutoStage::registerStage(&pipeline1_s);

int main(int argc, char *argv[]) {
    srand(123);
    std::cout << "srand: 123\n";

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
        cout << "\t--to (tiling only: generate tile images "
             << "without executing)" << endl;
        cout << "\t--nt (execute full image without tiling, overriding "
             << "any other tiling parameter)" << endl;
        cout << "\t--nhs (no halide scheduling: pipeline will execute "
             << " serially when selected)" << endl;
        cout << "\t--nic (no IWPP computing)" << endl;

        cout << "=== Hybrid execution options:" << endl;
        cout << "\t-a <execution option>" << endl;
        cout << "\t\tValues (default=0):" << endl;
        cout << "\t\t0: CPU-only execution" << endl;
        cout << "\t\t1: GPU-only execution (must run with -c 0)" << endl;
        cout << "\t\t2: Hybrid execution (must run with -h)" << endl;

        cout << "\t--gp <float value of PATS: gpu/cpu (default=1)>" << endl;

        cout << "\t--gn <gpu tiler multiplier (default=1)>" << endl;
        cout << "\t--gc <multi-gpu count (default=1)>" << endl;

        cout << "\t--mp <CPU thread count (default=0 i.e., all)>" << endl;

        cout << "=== Pre-tiler (PT) options:" << endl;
        cout << "\t--npt (without pre-tiler)" << endl;

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
        cout << "\t\t8: TILING1D" << endl;
        cout << "\t\t9: HIER_FG_BR" << endl;

        cout << "\t-f <dense cost function>" << endl;
        cout << "\t\tValues (default=0):" << endl;
        cout << "\t\t0: THRS" << endl;
        cout << "\t\t1: MV_THRS_AREA" << endl;
        cout << "\t\t2: TILED_THRS" << endl;
        cout << "\t\t3: LGB_MODEL" << endl;

        cout << "\t-m <execBias>/<loadBias> (default=1/100)" << endl;

        cout << "=== Background tiling (BG) options:" << endl;
        cout << "\t-k <background tiling algorithm>" << endl;
        cout << "\t\tValues (default=2):" << endl;
        cout << "\t\t2: LIST_ALG_EXPECT" << endl;
        cout << "\t\t1: LIST_ALG_HALF" << endl;

        cout << "=== Run-only partition options:" << endl;
        cout << "\t--RO <xi> <xo> <yi> <yo>" << endl;
        cout << "\t   This option overrides all others." << endl;
        cout << "\t   The tile marked by its coordinates is executed." << endl;

        // cout << "\t-xb (sort background tiles with area cost function)" <<
        // endl;

        // cout << "\t-tb (tile background tiles with area cost function)" <<
        // endl;

        exit(0);
    }

    // Configure max CPU thread count for Halide
    if (findArgPos("--mp", argc, argv) != -1) {
        setenv("HL_NUM_THREADS", argv[findArgPos("--mp", argc, argv) + 1], 1);
    }

    // Input images
    std::string Ipath = std::string(argv[1]);

    // Number of cpu threads
    int cpuThreads = 0;
    if (findArgPos("-c", argc, argv) != -1) {
        cpuThreads = atoi(argv[findArgPos("-c", argc, argv) + 1]);
    }

    // Number of gpu threads
    int gpuThreads = 0;
    if (findArgPos("-g", argc, argv) != -1) {
        gpuThreads = atoi(argv[findArgPos("-g", argc, argv) + 1]);
    }

    // Number of expected dense tiles for irregular tiling
    int nTiles = 1;
    if (findArgPos("-t", argc, argv) != -1) {
        nTiles = atoi(argv[findArgPos("-t", argc, argv) + 1]);
    }

    int border = 0;
    if (findArgPos("-b", argc, argv) != -1) {
        border = atoi(argv[findArgPos("-b", argc, argv) + 1]);
    }

    // Threshold cost function parameters
    int bgThr        = 150;
    int erode_param  = 4;
    int dilate_param = 2;
    if (findArgPos("-p", argc, argv) != -1) {
        std::string params = argv[findArgPos("-p", argc, argv) + 1];
        std::size_t l      = params.find_last_of("/");
        dilate_param       = atoi(params.substr(l + 1).c_str());
        params             = params.substr(0, l);
        l                  = params.find_last_of("/");
        erode_param        = atoi(params.substr(l + 1).c_str());
        bgThr              = atoi(params.substr(0, l).c_str());
    }

    // Tiling only
    bool tilingOnly = false;
    if (findArgPos("--to", argc, argv) != -1) {
        tilingOnly = true;
    }

    // No tiling execution
    bool noTiling = false;
    if (findArgPos("--nt", argc, argv) != -1) {
        noTiling = true;
    }

    int halNoSched = 0;
    if (findArgPos("--nhs", argc, argv) != -1) {
        halNoSched = 1;
    }

    int noIrregularComp = 0;
    if (findArgPos("--nic", argc, argv) != -1) {
        noIrregularComp = 1;
    }

    // Hybrid tiling algorithm
    HybridExec_t hybridExec = CPU_ONLY;
    if (findArgPos("-a", argc, argv) != -1) {
        hybridExec = static_cast<HybridExec_t>(
            atoi(argv[findArgPos("-a", argc, argv) + 1]));
    }

    // GPU PATS parameter
    float gpc = 1;
    if (findArgPos("--gp", argc, argv) != -1) {
        gpc = atof(argv[findArgPos("--gp", argc, argv) + 1]);
    }

    // GPU tiles multiplier
    int gn = 1;
    if (findArgPos("--gn", argc, argv) != -1) {
        gn = atoi(argv[findArgPos("--gn", argc, argv) + 1]);
    }

    int gpu_count = 1;
    if (findArgPos("--gc", argc, argv) != -1) {
        gpu_count = atoi(argv[findArgPos("--gc", argc, argv) + 1]);
    }

    // No pre-tiling execution
    bool preTiling = true;
    if (findArgPos("--npt", argc, argv) != -1) {
        preTiling = false;
    }

    // Dense tiling algorithm
    TilerAlg_t denseTilingAlg = FIXED_GRID_TILING;
    if (findArgPos("-d", argc, argv) != -1) {
        denseTilingAlg = static_cast<TilerAlg_t>(
            atoi(argv[findArgPos("-d", argc, argv) + 1]));
    }

    // Dense cost function used
    CostFunction_t denseCostf = THRS;
    if (findArgPos("-f", argc, argv) != -1) {
        denseCostf = static_cast<CostFunction_t>(
            atoi(argv[findArgPos("-f", argc, argv) + 1]));
    }

    // Threshold cost function parameters
    int execBias = 1;
    int loadBias = 100;
    if (findArgPos("-m", argc, argv) != -1) {
        std::string params = argv[findArgPos("-m", argc, argv) + 1];
        std::size_t l      = params.find_last_of("/");
        execBias           = atoi(params.substr(0, l).c_str());
        loadBias           = atoi(params.substr(l + 1).c_str());
    }

    // BG tiling algorithm
    TilerAlg_t bgTilingAlg = LIST_ALG_EXPECT;
    if (findArgPos("-k", argc, argv) != -1) {
        bgTilingAlg = static_cast<TilerAlg_t>(
            atoi(argv[findArgPos("-k", argc, argv) + 1]));
    }

    if (noTiling && tilingOnly) {
        cout << "Options 'tiling-only' and 'no tiling' "
             << "are mutually exclusive" << endl;
        exit(0);
    }

    // Asserts logical setups:
    // 1. Hybrid execution must have "-h"
    // 2. CPU_ONLY execution cannot have gpu threads
    // 3. GPU_ONLY execution cannot have cpu threads

    switch (hybridExec) {
        case CPU_ONLY:
            argv[findArgPos("-g", argc, argv) + 1][0] = '0';
            cout << "[main] WARNING: GPU threads not created "
                 << "due to CPU_ONLY execution." << endl;
            break;
        case GPU_ONLY:
            argv[findArgPos("-c", argc, argv) + 1][0] = '0';
            cout << "[main] WARNING: CPU threads not created "
                 << "due to GPU_ONLY execution." << endl;
            break;
        case HYBRID:
            if (findArgPos("-h", argc, argv) == -1) {
                cout << "[main] Hybrid execution must be with halide queue. "
                     << "Please run with -h." << endl;
                exit(0);
            }
            break;
    }

    SysEnv sysEnv;
    sysEnv.startupSystem(argc, argv, "libautostage.so");

#ifdef PROFILING
    long fullExecT1 = Util::ClockGetTime();
#endif

    // Input parameters
    unsigned char blue                  = 200;
    unsigned char green                 = 200;
    unsigned char red                   = 200;
    double        T1                    = 1.0;
    double        T2                    = 2.0;
    unsigned char G1                    = 50;
    unsigned char G2                    = 100;
    int           minSize               = 10;
    int           maxSize               = 100;
    int           fillHolesConnectivity = 4;
    int           reconConnectivity     = 4;
    // 19x19
    int disk19raw_width           = 19;
    int disk19raw_size            = disk19raw_width * disk19raw_width;
    int disk19raw[disk19raw_size] = {
        0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0};

    // 3x3
    int se3raw_width        = 3;
    int se3raw_size         = se3raw_width * se3raw_width;
    int se3raw[se3raw_size] = {1, 1, 1, 1, 1, 1, 1, 1, 1};

#ifdef PROFILING
    long tilingT1 = Util::ClockGetTime();
#endif

    // Uppercase coordinates are from real-sized image
    // Lowercase coordinates are from small res image
    long ro_xi, ro_xo, ro_yi, ro_yo, ro_Xi, ro_Xo, ro_Yi, ro_Yo;
    if (findArgPos("--RO", argc, argv) != -1) {
        ro_xi = atoi(argv[findArgPos("--RO", argc, argv) + 1]);
        ro_xo = atoi(argv[findArgPos("--RO", argc, argv) + 2]);
        ro_yi = atoi(argv[findArgPos("--RO", argc, argv) + 3]);
        ro_yo = atoi(argv[findArgPos("--RO", argc, argv) + 4]);
        ro_Xi = atoi(argv[findArgPos("--RO", argc, argv) + 5]);
        ro_Xo = atoi(argv[findArgPos("--RO", argc, argv) + 6]);
        ro_Yi = atoi(argv[findArgPos("--RO", argc, argv) + 7]);
        ro_Yo = atoi(argv[findArgPos("--RO", argc, argv) + 8]);

        RegionTemplate        *rtOut = newRT("rtOut");
        SingleTileRTCollection singleTileTiler("input", "input", Ipath, ro_xi,
                                               ro_xo, ro_yi, ro_yo);
        singleTileTiler.addImage(Ipath);
        singleTileTiler.tileImages();
        singleTileTiler.generateDRs(tilingOnly);

        for (int i = 0; i < singleTileTiler.getNumRTs(); i++) {
            auto stage0 = new RTF::AutoStage(
                {singleTileTiler.getRT(i).second, rtOut},
                {new ArgumentInt(i), new ArgumentInt(blue),
                 new ArgumentInt(green), new ArgumentInt(red),
                 new ArgumentInt(0), new ArgumentInt(disk19raw_width),
                 new ArgumentIntArray(disk19raw, disk19raw_size),
                 new ArgumentInt(G1), new ArgumentInt(se3raw_width),
                 new ArgumentIntArray(se3raw, se3raw_size),
                 new ArgumentInt(halNoSched), new ArgumentInt(noIrregularComp),
                 new ArgumentInt(gpu_count)},
                {ro_Yo - ro_Yi, ro_Xo - ro_Xi}, {&pipeline1_s},
                tgt_merge(hybridExec), i);
            stage0->genStage(sysEnv);
        }
        sysEnv.startupExecution();
        for (int i = 0; i < rtOut->getNumDataRegions(); i++) {
            cout << "===== dr: " << rtOut->getDataRegion(i)->getName() << "\n";
            rtOut->getDataRegion(i)->printRoi();
        }
        std::cout << "Done executing\n";

        // delete rtOut;
        sysEnv.finalizeSystem();
        return 0;
    }

    // Tilers
    TiledRTCollection *denseTiler;
    TiledRTCollection *bgTiler;

    // Tiling cost functions
    BGMasker     *bgm = new ThresholdBGMasker(bgThr, dilate_param, erode_param);
    CostFunction *denseCostFunc;
    if (denseCostf == THRS) {
        denseCostFunc =
            new ThresholdBGCostFunction(static_cast<ThresholdBGMasker *>(bgm));
    } else if (denseCostf == MV_THRS_AREA) {
        denseCostFunc = new MultiObjCostFunction(
            static_cast<ThresholdBGMasker *>(bgm), execBias, loadBias);
    } else if (denseCostf == TILED_THRS) {
        int xRes      = 20;
        int yRes      = 20;
        denseCostFunc = new PrePartThresBGCostFunction(bgThr, dilate_param,
                                                       erode_param, Ipath);
    } else if (denseCostf == LGB_MODEL) {
        denseCostFunc = new LGBCostFunction(bgThr, dilate_param, erode_param);
    }
    CostFunction *bgCostFunc = new AreaCostFunction();

    // Create extra tiles for gpu
    if (hybridExec == GPU_ONLY) {
        nTiles *= gn;
    }

    // Verifies for hybrid execution
    if (hybridExec == HYBRID) {
        float cpuPATS = 1;
        float gpuPATS = gpc;
        denseTiler    = new HybridDenseTiledRTCollection(
               "input", "input", Ipath, border, denseCostFunc, bgm, denseTilingAlg,
               nTiles, nTiles * gn, cpuPATS, gpuPATS, true);
    } else {
        // Creates dense tiling collection
        if (noTiling)
            denseTiler = new TiledRTCollection("input", "input", Ipath, border,
                                               denseCostFunc);
        else if (denseTilingAlg == FIXED_GRID_TILING)
            denseTiler = new RegTiledRTCollection(
                "input", "input", Ipath, nTiles, border, denseCostFunc);
        else if (denseTilingAlg == TILING1D)
            denseTiler = new TiledRTCollection("input", "input", Ipath, border,
                                               denseCostFunc, nTiles);
        else
            denseTiler = new IrregTiledRTCollection(
                "input", "input", Ipath, border, denseCostFunc, bgm,
                denseTilingAlg, nTiles, true);
    }

    // Performs pre-tiling, if required
    BGPreTiledRTCollection preTiler("input", "input", Ipath, border,
                                    denseCostFunc, bgm);
    long                   tilingT12 = Util::ClockGetTime();
    if (preTiling && !noTiling) {
        // Performs actual tiling
        preTiler.addImage(Ipath);
        preTiler.tileImages(tilingOnly);

        // Send outputs to dense tiling
        denseTiler->setPreTiles(preTiler.getDense());
    }

    long tilingT13 = Util::ClockGetTime();
    // Performs dense tiling
    denseTiler->addImage(Ipath);
    denseTiler->tileImages(tilingOnly);

    long tilingT14 = Util::ClockGetTime();
    // // BG tiling
    // if (preTiling && !noTiling) {
    //     // Calculates the number of expected tiles as a multiple of nTiles
    //     int bgTilesExpected =
    //         (std::ceil((preTiler.getBgSize() + denseTiler->getDenseSize()) /
    //                    nTiles)) *
    //         nTiles;
    //     if (hybridExec == HYBRID) {
    //         // bgTiler = new HybridDenseTiledRTCollection(
    //         //     "input", "input", Ipath, border, bgCostFunc, bgm,
    //         //     bgTilingAlg, bgTilesExpected, 0, 1, 1);
    //         bgTiler = new HybridDenseTiledRTCollection(
    //             "input", "input", Ipath, border, bgCostFunc, bgm,
    //             bgTilingAlg, nTiles, nTiles * gn, 1, 1);
    //     } else {
    //         bgTiler = new IrregTiledRTCollection("input", "input", Ipath,
    //                                              border, bgCostFunc, bgm,
    //                                              bgTilingAlg,
    //                                              bgTilesExpected);
    //     }

    //     std::map<std::string, std::list<cv::Rect_<int64_t>>> bgTiles(
    //         preTiler.getBg());

    //     for (string img : std::list<string>({Ipath})) {
    //         bgTiles[img].insert(bgTiles[img].end(),
    //                             denseTiler->getBgTilesBase()[img].begin(),
    //                             denseTiler->getBgTilesBase()[img].end());
    //     }
    //     bgTiler->setPreTiles(bgTiles);

    //     // Performs BG tiling and adds results to dense
    //     bgTiler->addImage(Ipath);
    //     bgTiler->tileImages(tilingOnly);

    //     denseTiler->addTiles(bgTiler->getTilesBase());
    //     denseTiler->addTargets(bgTiler->getTargetsBase());
    // }

    long tilingT15 = Util::ClockGetTime();
    // Generates tiles and converts it to vector

    // Update cost function for getting real values
    if (denseCostf == TILED_THRS) {
        denseTiler->setCostFunction(
            new ThresholdBGCostFunction(static_cast<ThresholdBGMasker *>(bgm)));
    }
    denseTiler->generateDRs(tilingOnly);
    std::vector<cv::Rect_<int>>   tiles;
    std::list<cv::Rect_<int64_t>> l = denseTiler->getTiles()[0];
    cout << "[main] Tiles generated: " << l.size() << endl;
    for (cv::Rect_<int64_t> tile : l) {
        std::cout << tile.x << ":" << tile.width << "," << tile.y << ":"
                  << tile.height << std::endl;
        tiles.emplace_back(tile);
    }

#ifdef PROFILING
    long tilingT2 = Util::ClockGetTime();
#endif

    // Create an instance of the two stages for each image tile pair
    // and also send them for execution
    RegionTemplate *rtOut = newRT("rtOut");
    // rtOut->setMergeLater();
    // rtOut->setMergeOutputSize(denseTiler->getImageSize().height,
    //                           denseTiler->getImageSize().width);

    // Params are 4 coordinates (rect) of tile, and height/width of full image
    std::vector<ArgumentBase *> mergeParams(tiles.size() * 4 + 2);
    mergeParams[mergeParams.size() - 2] =
        new ArgumentInt(denseTiler->getImageSize().height);
    mergeParams[mergeParams.size() - 1] =
        new ArgumentInt(denseTiler->getImageSize().width);
    for (int i = 0; i < tiles.size(); i++) {
        mergeParams[i * 4 + 0] = new ArgumentInt(tiles[i].x);
        mergeParams[i * 4 + 1] = new ArgumentInt(tiles[i].y);
        mergeParams[i * 4 + 2] = new ArgumentInt(tiles[i].width);
        mergeParams[i * 4 + 3] = new ArgumentInt(tiles[i].height);
    }

    // std::list<RTF::AutoStage *> stages;
    for (int i = 0; i < denseTiler->getNumRTs(); i++) {
        auto stage0 = new RTF::AutoStage(
            {denseTiler->getRT(i).second, rtOut},
            {new ArgumentInt(i), new ArgumentInt(blue), new ArgumentInt(green),
             new ArgumentInt(red), new ArgumentInt(0),
             new ArgumentInt(disk19raw_width),
             new ArgumentIntArray(disk19raw, disk19raw_size),
             new ArgumentInt(G1), new ArgumentInt(se3raw_width),
             new ArgumentIntArray(se3raw, se3raw_size),
             new ArgumentInt(halNoSched), new ArgumentInt(noIrregularComp),
             new ArgumentInt(gpu_count)},
            {tiles[i].height, tiles[i].width}, {&pipeline1_s},
            tgt(hybridExec, denseTiler->getTileTarget(i)), i);
        stage0->genStage(sysEnv);
    }

    sysEnv.startupExecution();
    for (int i = 0; i < rtOut->getNumDataRegions(); i++) {
        cout << "===== dr: " << rtOut->getDataRegion(i)->getName() << "\n";
        rtOut->getDataRegion(i)->printRoi();
    }
    // delete rtOut;
    sysEnv.finalizeSystem();

#ifdef PROFILING
    long fullExecT2 = Util::ClockGetTime();
    cout << "[PROFILING][TILES] " << tiles.size() << endl;
    cout << "[PROFILING][TILING_TIME] " << (tilingT2 - tilingT1) << endl;
    cout << "[PROFILING][TILING_PREP_CFNC_TIME] " << (tilingT12 - tilingT1)
         << endl;
    cout << "[PROFILING][TILING_PRE_TILING_TIME] " << (tilingT13 - tilingT12)
         << endl;
    cout << "[PROFILING][TILING_DENSE_TILING_TIME] " << (tilingT14 - tilingT13)
         << endl;
    cout << "[PROFILING][TILING_BG_TILING_TIME] " << (tilingT15 - tilingT14)
         << endl;
    cout << "[PROFILING][TILING_GEN_TIME] " << (tilingT2 - tilingT15) << endl;
    cout << "[PROFILING][FULL_TIME] " << (fullExecT2 - fullExecT1) << endl;
#endif

    return 0;
}
