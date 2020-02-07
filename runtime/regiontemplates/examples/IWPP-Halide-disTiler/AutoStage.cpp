#include "AutoStage.h"

RTF::Internal::AutoStage::AutoStage(std::vector<RegionTemplate*> rts, 
    std::vector<int64_t> out_shape, std::map<Target_t, HalGen*> schedules, 
    std::vector<ArgumentBase*> params, int tileId) {

    this->setComponentName("AutoStage");

    this->out_shape = out_shape;
    this->tileId = tileId;

    // Add parameters
    for (ArgumentBase* arg : params)
        this->addArgument(arg);

    // Gets the names of the registered stages
    for (std::pair<Target_t, HalGen*> s : schedules) {
        this->schedules[s.first] = s.second->getName();
        this->addTaskTarget(s.first);
    }

    std::string dr_name;

    // Populates the list of RTs names while also adding them to the RTPCB
    // Just the inputs here
    for (int i=0; i<rts.size()-1; i++) {
        dr_name = (tileId==-1?rts[i]->getName() : "t" + std::to_string(tileId));
        rts_names.emplace_back(rts[i]->getName());
        this->addRegionTemplateInstance(rts[i], rts[i]->getName());
        this->addInputOutputDataRegion(rts[i]->getName(), dr_name, 
            RTPipelineComponentBase::INPUT);
    }
    
    // Add the output RT
    RegionTemplate* last_rt = rts[rts.size()-1];
    dr_name = (tileId==-1 ? last_rt->getName() : "t" + std::to_string(tileId));
    rts_names.emplace_back(last_rt->getName());
    this->addRegionTemplateInstance(last_rt, last_rt->getName());
    this->addInputOutputDataRegion(last_rt->getName(), dr_name, 
        RTPipelineComponentBase::OUTPUT);
};

int RTF::Internal::AutoStage::run() {
    // Assemble input/output cv::Mat list for execution
    // Starts with the inputs
#ifdef DEBUG
    std::cout << "[Internal::AutoStage] running" << std::endl;
#endif
    std::vector<DenseDataRegion2D*> dr_ios;
    std::string drName;
    for (int i=0; i<this->rts_names.size()-1; i++) {
        RegionTemplate* rt = this->getRegionTemplateInstance(
            this->rts_names[i]);
        drName = tileId==-1 ? rt->getName() : "t"+std::to_string(this->tileId);
        DenseDataRegion2D* dr = dynamic_cast<DenseDataRegion2D*>(
            rt->getDataRegion(drName));
        dr_ios.emplace_back(dr);
    }

    // Output buffer must be pre-allocated for the halide pipeline
    cv::Mat* cvOut = new cv::Mat(this->out_shape[0], 
        this->out_shape[1], CV_8U);

    // Assign output mat to the correct output RT
    RegionTemplate* rtOut = this->getRegionTemplateInstance(
        this->rts_names[this->rts_names.size()-1]);
    DenseDataRegion2D *drOut = new DenseDataRegion2D();
    drName = tileId==-1 ? rtOut->getName() : "t"+std::to_string(this->tileId);
    drOut->setName(drName);
    drOut->setId(rtOut->getName());
    drOut->setData(*cvOut);
    rtOut->insertDataRegion(drOut);

    dr_ios.emplace_back(drOut);
#ifdef DEBUG
    std::cout << "[Internal::AutoStage] creating task" << std::endl;
#endif

    // Assemble a schedule map with the local pointers for the halide functions
    std::map<Target_t, HalGen*> local_schedules;
    for (std::pair<Target_t, std::string> s : this->schedules) {
        local_schedules[s.first] = RTF::AutoStage::retrieveStage(s.second);
    }

    // Anonymous class for implementing the current stage's task
    struct _Task : public Task {
        std::map<Target_t, HalGen*> schedules;
        std::vector<DenseDataRegion2D*> dr_ios;
        std::vector<ArgumentBase*> params;
        
        _Task(std::map<Target_t, HalGen*> schedules, 
            std::vector<DenseDataRegion2D*>& dr_ios, 
            std::vector<ArgumentBase*> params):
        schedules(schedules), dr_ios(dr_ios), params(params) {};

        bool run(int procType, int tid=0) {
            // Generates the input/output list of cv::mat
#ifdef DEBUG
            std::cout << "[Internal::AutoStage::_Task] realizing " 
                << schedules.begin()->second->getName() << std::endl;
#endif
            std::vector<cv::Mat> im_ios;
            bool aborted = false;
            for (int i=0; i<this->dr_ios.size(); i++) {
                im_ios.emplace_back(cv::Mat(this->dr_ios[i]->getData()));
                if (this->dr_ios[i]->aborted()) {
                    aborted = true;
                    break;
                }
            }

            // Executes the halide stage if not aborted
            if (!aborted)
                aborted = schedules[procType]->realize(im_ios, procType, params);
            else {
                std::string abortStr = "[Internal::AutoStage::_Task]";
                abortStr += " Aborted exec of tiles with size ";
                for (int i=0; i<this->dr_ios.size(); i++) {
                    // set abort flag for all further data regions
                    this->dr_ios[i]->abort();

                    abortStr += this->dr_ios[i]->getData().rows;
                    abortStr += "x";
                    abortStr += this->dr_ios[i]->getData().cols;
                    abortStr += " ";
                }
                std::cout << abortStr << std::endl;
            }

                

            // Assigns the output mat to its DataRegion
#ifdef DEBUG
            std::cout << "[Internal::AutoStage::_Task] realized " 
                << std::endl;
#endif
        }
    }* currentTask = new _Task(local_schedules, dr_ios, this->getArguments());

    // Set targets for this task
    for (std::pair<Target_t, std::string> s : this->schedules) {
        currentTask->addTaskTarget(s.first);
    }

#ifdef DEBUG
    std::cout << "[Internal::AutoStage] sending task for execution" << std::endl;
#endif
    this->executeTask(currentTask);
}

RTF::Internal::AutoStage* RTF::AutoStage::genStage(SysEnv& sysEnv) {
    // Generate current stage if it was not already generated
    if (this->generatedStage == NULL) {
        this->generatedStage = new Internal::AutoStage(rts, 
            out_shape, schedules, params, this->tileId);
    } else {
        return this->generatedStage;
    }

    // Generate dependent stages
    for (AutoStage* dep : deps) {
        // Generate the dependent stage while also adding it as a
        // dependency for the current stage
        RTF::Internal::AutoStage* internalDep = dep->genStage(sysEnv);
        this->generatedStage->addDependency(internalDep->getId());
    }
    sysEnv.executeComponent(this->generatedStage);
    return this->generatedStage;
}

void RTF::AutoStage::execute(int argc, char** argv) {
    // Verifies if this stage was the last stage of a pipeline
    if (!this->last_stage) {
        std::cout << "[RTF::AutoStage::execute] this is not the last stage." 
            << std::endl;
        exit(-1);
    }

    std::string shd_lib_name = "libautostage.so";

#ifdef DEBUG
    cout << "[AutoStage::execute] starting sys" << endl;
#endif
    SysEnv sysEnv;
    sysEnv.startupSystem(argc, argv, shd_lib_name);

    // Generate all stages and send them for execution
    this->genStage(sysEnv);

    // Startup execution is this is the final stage of the pipeline
#ifdef DEBUG
    cout << "[AutoStage::execute] executing pipeline" << endl;
#endif
    sysEnv.startupExecution();
    sysEnv.finalizeSystem();
}

// Create the component factory
PipelineComponentBase* componentFactoryAutoStage() {
    return new RTF::Internal::AutoStage();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister(
    "AutoStage", &componentFactoryAutoStage);

// Local register of halide stages (static into AutoStage)
std::map<std::string, RTF::HalGen*> RTF::AutoStage::stagesReg;

bool RTF::AutoStage::registerStage(HalGen* stage) {
    // If the parameter stage is already registered, it will be ignored
    if (RTF::AutoStage::stagesReg.find(stage->getName()) 
            == RTF::AutoStage::stagesReg.end()) {
        stagesReg[stage->getName()] = stage;
        return true;
    }
    return false;
}

RTF::HalGen* RTF::AutoStage::retrieveStage(std::string name) {
    return RTF::AutoStage::stagesReg.find(name)->second;
}
