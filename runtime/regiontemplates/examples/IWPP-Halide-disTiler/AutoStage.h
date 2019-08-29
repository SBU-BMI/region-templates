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

#define DATA_T uint8_t

namespace RTF {

// Should use ExecEngineConstants::GPU ... 
typedef int Target_t;

// Interface for realizing halide pipelines inside RTF
struct HalGen {
    virtual std::string getName() = 0;
    virtual Target_t getTarget() = 0;
    static std::vector<ArgumentBase*> _dft;
    virtual void realize(std::vector<cv::Mat>& im_ios, 
        std::vector<ArgumentBase*>& params = _dft) = 0;
};

namespace Internal {

class AutoStage : public RTPipelineComponentBase {
    std::vector<std::string> rts_names;
    std::vector<int> out_shape; // Rows at 0, cols at 1
    std::map<Target_t, std::string> schedules;
    std::vector<ArgumentBase*> params;


public:
    // Empty constructor for cloning and for the ComponentFactory 
    AutoStage() {
        this->setComponentName("AutoStage");
    };
    // Regular user constructor
    AutoStage(std::vector<RegionTemplate*> rts, 
        std::vector<int> out_shape, std::map<Target_t, HalGen*> schedules, 
        std::vector<ArgumentBase*> params) : out_shape(out_shape),
        params(params) {

        this->setComponentName("AutoStage");

        // Gets the names of the registered stages
        for (std::pair<Target_t, HalGen*> s : schedules) {
            this->schedules[s.first] = s.second->getName();
        }

        // Populates the list of RTs names while also adding them to the RTPCB
        // Just the inputs here
        for (int i=0; i<rts.size()-1; i++) {
            rts_names.emplace_back(rts[i]->getName());
            this->addRegionTemplateInstance(rts[i], rts[i]->getName());
            this->addInputOutputDataRegion(rts[i]->getName(), rts[i]->getName(), 
                RTPipelineComponentBase::INPUT);
        }
        
        // Add the output RT
        RegionTemplate* last_rt = rts[rts.size()-1];
        rts_names.emplace_back(last_rt->getName());
        this->addRegionTemplateInstance(last_rt, last_rt->getName());
        this->addInputOutputDataRegion(last_rt->getName(), last_rt->getName(), 
            RTPipelineComponentBase::OUTPUT);
    };
    virtual ~AutoStage() {};

    // Serialization methods
    int serialize(char *buff);
    int deserialize(char *buff);
    int size();
    AutoStage* clone();

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
    std::vector<int> out_shape; // Rows at 0, cols at 1
    std::map<Target_t, HalGen*> schedules;
    std::list<AutoStage*> deps;
    
    // Internal RTF executable representation of this stage
    Internal::AutoStage* generatedStage; // RTF stage can only be generated once

    // Local register of halide functions
    friend class Internal::AutoStage;
    static std::map<std::string, HalGen*> stagesReg;

public:
    AutoStage(std::vector<RegionTemplate*> rts, 
        std::vector<ArgumentBase*> params, std::vector<int> out_shape, 
        std::list<HalGen*> schedules) : rts(rts), params(params), 
        out_shape(out_shape) {
        
        this->generatedStage = NULL;

        // Converts the schedules list to a map 
        // id'ed by the target of the schedule
        for (HalGen* hg : schedules) {
            this->schedules[hg->getTarget()] = hg;

            // Registers the stage locally (node wise) for later
            // retrieval for execution
            this->registerStage(hg);
        }
    }
    virtual ~AutoStage() {};

    // Creates a dependency bond
    void after(AutoStage* dep) {
        deps.emplace_back(dep);
    }

    // Generates the current stage as a PCB object, sends it to be
    // executed on the sysEnv while also generating recursively the
    // dependent stages. If called multiple times, only a single
    // PCB instance will be generated. This is the returned PCB pointer.
    Internal::AutoStage* genStage(SysEnv& sysEnv);

    // First implementation only has one stage
    void execute(int argc, char** argv);

    // Local stages register methods
    static void registerStage(HalGen* stage);
    static HalGen* retrieveStage(std::string name);
};

}; // namespace RTF


#endif // AUTO_STAGE_H
