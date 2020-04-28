#ifndef AUTO_STAGE_H
#define AUTO_STAGE_H

#include <stdint.h>
#include <string>
#include <vector>

#include "Halide.h"

#include "SysEnv.h"
#include "RegionTemplate.h"
#include "DenseDataRegion2D.h"
#include "RTPipelineComponentBase.h"
#include "Argument.h"
#include "ExecEngineConstants.h"

// #include "tilingAlgs/tilingUtil.h"
#include "TiledRTCollection.h"
#include "TiledMatCollection.h"
#include "BGPreTiledRTCollection.h"
#include "IrregTiledRTCollection.h"
#include "costFuncs/ThresholdBGCostFunction.h"
#include "costFuncs/ThresholdBGMasker.h"

namespace RTF {

// Should use ExecEngineConstants::GPU ... 
typedef int Target_t;

// Interface for realizing halide pipelines inside RTF
struct HalGen {
    virtual std::string getName() = 0;
    // virtual Target_t getTarget() = 0;
    static std::vector<ArgumentBase*> _dft; // default empty constructor variable
    // must return an aborted flag (if true, all following stages must be aborted)
    virtual bool realize(std::vector<cv::Mat>& im_ios, Target_t target, 
        std::vector<ArgumentBase*>& params = _dft) = 0;
};

namespace Internal {

class AutoStage : public RTPipelineComponentBase {
private:
    void localTileDRs(std::list<cv::Rect_<int64_t>>& tiles, 
    std::vector<std::vector<DenseDataRegion2D*>>& allTiles);

public:
    // Empty constructor for cloning and for the ComponentFactory 
    AutoStage() {
        this->setComponentName("AutoStage");
    };
    // Regular user constructor
    AutoStage(std::vector<RegionTemplate*> rts, std::vector<int64_t> out_shape, 
        std::map<Target_t, HalGen*> schedules, 
        std::vector<ArgumentBase*> params, int tileId);
    virtual ~AutoStage() {};

    // Overwritten Task::run method
    int run();
};
    
}; // namespace Internal

// Class used by the end user as a pipeline stage. It can initialize
// the sys env by itself and resides only on the manager/staging part
// of the user code. It is not executed on the RTF (see Internal::AutoStage).
class AutoStage {
    std::vector<RegionTemplate*> rts;
    std::vector<ArgumentBase*> params;
    std::vector<int64_t> out_shape; // Rows at 0, cols at 1
    std::map<Target_t, HalGen*> schedules;
    std::list<AutoStage*> deps;
    int tileId; // ID of which tile was sent for this stage from autoTiler
    bool last_stage;
    
    // Internal RTF executable representation of this stage
    Internal::AutoStage* generatedStage; // RTF stage can only be generated once

    // Local register of halide functions
    static std::map<std::string, HalGen*> stagesReg;

public:
    AutoStage(std::vector<RegionTemplate*> rts, 
        std::vector<ArgumentBase*> params, std::vector<int64_t> out_shape, 
        std::list<HalGen*> schedules, Target_t target, int tileId) 
            : rts(rts), params(params), out_shape(out_shape), 
            last_stage(true), tileId(tileId) {
        
        this->generatedStage = NULL;

        // Converts the schedules list to a map 
        // id'ed by the target of the schedule
        for (HalGen* hg : schedules) {
            this->schedules[target] = hg;

            // Registers the stage locally (node wise) for later
            // retrieval for execution
            this->registerStage(hg);
        }
    }
    // AutoStage(std::vector<RegionTemplate*> rts, 
    //     std::vector<ArgumentBase*> params, std::vector<int64_t> out_shape, 
    //     std::list<HalGen*> schedules, int tileId) : rts(rts), params(params), 
    //     out_shape(out_shape), last_stage(true), tileId(tileId) {
        
    //     this->generatedStage = NULL;

    //     // Converts the schedules list to a map 
    //     // id'ed by the target of the schedule
    //     for (HalGen* hg : schedules) {
    //         this->schedules[hg->getTarget()] = hg;

    //         // Registers the stage locally (node wise) for later
    //         // retrieval for execution
    //         this->registerStage(hg);
    //     }
    // }
    virtual ~AutoStage() {};

    // Creates a dependency bond
    void after(AutoStage* dep) {
        deps.emplace_back(dep);
        dep->last_stage = false;
    }

    // Generates the current stage as a PCB object, sends it to be
    // executed on the sysEnv while also generating recursively the
    // dependent stages. If called multiple times, only a single
    // PCB instance will be generated. This is the returned PCB pointer.
    Internal::AutoStage* genStage(SysEnv& sysEnv);

    // First implementation only has one stage
    void execute(int argc, char** argv);

    // Local stages register methods
    static bool registerStage(HalGen* stage);
    static HalGen* retrieveStage(std::string name);
};

}; // namespace RTF


#endif // AUTO_STAGE_H
