#include "AutoStage.h"

int RTF::Internal::AutoStage::serialize(char *buff) {
    int serialized_bytes = RTPipelineComponentBase::serialize(buff);

    // packs the rts_names vector size
    int num_rts_names = this->rts_names.size();
    memcpy(buff+serialized_bytes, &num_rts_names, sizeof(int));
    serialized_bytes += sizeof(int);

    // packs the rts_names vector
    for (int i=0; i<num_rts_names; i++) {
        // packs the string size
        int rts_name_size = this->rts_names[i].size();
        memcpy(buff+serialized_bytes, &rts_name_size, sizeof(int));
        serialized_bytes += sizeof(int);
        
        // packs the string itself
        memcpy(buff+serialized_bytes, this->rts_names[i].c_str(), 
            sizeof(char)*rts_name_size);
        serialized_bytes += sizeof(char)*rts_name_size;
    }

    // packs the out_shape vector size
    int num_out_shape = this->out_shape.size();
    memcpy(buff+serialized_bytes, &num_out_shape, sizeof(int));
    serialized_bytes += sizeof(int);

    // packs the out_shape vector
    for (int i=0; i<num_out_shape; i++) {
        memcpy(buff+serialized_bytes, &out_shape[i], sizeof(int));
        serialized_bytes += sizeof(int);
    }

    // packs the schedules map size
    int num_schedules = this->schedules.size();
    memcpy(buff+serialized_bytes, &num_schedules, sizeof(int));
    serialized_bytes += sizeof(int);

    // packs the schedules map
    for (std::pair<Target_t, HalGen*> s : schedules) {
        // packs the HalGen target
        memcpy(buff+serialized_bytes, &s.first, sizeof(int));
        serialized_bytes += sizeof(int);

        // packs the halide function reference
        // Obs: this is a reference to a static class instantiation
        memcpy(buff+serialized_bytes, &s.second, sizeof(HalGen*));
        serialized_bytes += sizeof(HalGen*);
    }

    return serialized_bytes;
}

int RTF::Internal::AutoStage::deserialize(char *buff) {
    int deserialized_bytes = RTPipelineComponentBase::deserialize(buff);

    // unpacks the rts_names vector size
    int num_rts_names;
    memcpy(&num_rts_names, buff+deserialized_bytes, sizeof(int));
    deserialized_bytes += sizeof(int);

    // unpacks the rts_names vector
    for (int i=0; i<num_rts_names; i++) {
        // unpacks the string size
        int rts_name_size;
        memcpy(&rts_name_size, buff+deserialized_bytes, sizeof(int));
        deserialized_bytes += sizeof(int);
        
        // unpacks the string itself
        char rt_name[rts_name_size+1];
        rt_name[rts_name_size] = '\0';
        memcpy(rt_name, buff+deserialized_bytes, sizeof(char)*rts_name_size);
        this->rts_names.emplace_back(std::string(rt_name));
        deserialized_bytes += sizeof(char)*rts_name_size;
    }

    // unpacks the out_shape vector size
    int num_out_shape;
    memcpy(&num_out_shape, buff+deserialized_bytes, sizeof(int));
    deserialized_bytes += sizeof(int);

    // unpacks the out_shape vector
    for (int i=0; i<num_out_shape; i++) {
        int out_shape_val;
        memcpy(buff+deserialized_bytes, &out_shape_val, sizeof(int));
        deserialized_bytes += sizeof(int);
        this->out_shape.emplace_back(out_shape_val);
    }

    // unpacks the schedules map size
    int num_schedules;
    memcpy(&num_schedules, buff+deserialized_bytes, sizeof(int));
    deserialized_bytes += sizeof(int);

    // unpacks the schedules map
    for (int i=0; i<num_schedules; i++) {
        // unpacks the HalGen target
        Target_t target;
        memcpy(&target, buff+deserialized_bytes, sizeof(int));
        deserialized_bytes += sizeof(int);

        // unpacks the halide function reference
        // Obs: this is a reference to a static class instantiation
        HalGen* halRef;
        memcpy(&halRef, buff+deserialized_bytes, sizeof(HalGen*));
        deserialized_bytes += sizeof(HalGen*);

        this->schedules[target] = halRef;
    }

    return deserialized_bytes;
}

int RTF::Internal::AutoStage::size() {
    int size = RTPipelineComponentBase::size();

    // packs the rts_names vector size
    size += sizeof(int);

    // packs the rts_names vector
    for (int i=0; i<this->rts_names.size(); i++) {
        // packs the string size
        size += sizeof(int);
        
        // packs the string itself
        size += sizeof(char)*this->rts_names[i].size();
    }

    // packs the out_shape vector size
    size += sizeof(int);

    // packs the out_shape vector
    for (int i=0; i<this->out_shape.size(); i++) {
        size += sizeof(int);
    }

    // packs the schedules map size
    size += sizeof(int);

    // packs the schedules map
    for (std::pair<Target_t, HalGen*> s : schedules) {
        // packs the HalGen target
        size += sizeof(int);

        // packs the halide function reference size
        size += sizeof(HalGen*);
    }

    return size;
}

RTF::Internal::AutoStage* RTF::Internal::AutoStage::clone() {
    AutoStage* retValue = new AutoStage();
    int size = this->size();
    char *buff = new char[size];
    this->serialize(buff);
    retValue->deserialize(buff);
    delete buff;
    return retValue;
}

int RTF::Internal::AutoStage::run() {
    // Assemble input/output cv::Mat list for execution
    // Starts with the inputs
    std::vector<DenseDataRegion2D*> dr_ios;
    for (int i=0; i<this->rts_names.size()-1; i++) {
        RegionTemplate* rt = this->getRegionTemplateInstance(
            this->rts_names[i]);
        DenseDataRegion2D* dr = dynamic_cast<DenseDataRegion2D*>(
            rt->getDataRegion(rt->getName()));
        dr_ios.emplace_back(dr);
    }

    // Output buffer must be pre-allocated for the halide pipeline
    cv::Mat* cvOut = new cv::Mat(this->out_shape[0], 
        this->out_shape[1], CV_8U);

    // Assign output mat to the correct output RT
    RegionTemplate* rtOut = this->getRegionTemplateInstance(
        this->rts_names[this->rts_names.size()-1]);
    DenseDataRegion2D *drOut = new DenseDataRegion2D();
    drOut->setName(rtOut->getName());
    drOut->setData(*cvOut);
    rtOut->insertDataRegion(drOut);

    dr_ios.emplace_back(drOut);

    // Anonymous class for implementing the current stage's task
    struct _Task : public Task {
        std::map<Target_t, HalGen*> schedules;
        std::vector<DenseDataRegion2D*> dr_ios;
        std::vector<ArgumentBase*> params;
        
        _Task(std::map<Target_t, HalGen*> schedules, 
            std::vector<DenseDataRegion2D*> dr_ios, 
            std::vector<ArgumentBase*> params):
        schedules(schedules), dr_ios(dr_ios), params(params) {};

        bool run(int procType, int tid=0) {
            // Generates the input/output list of cv::mat
            std::vector<cv::Mat> im_ios;
            for (DenseDataRegion2D* ddr2d : dr_ios) {
                im_ios.emplace_back(cv::Mat(ddr2d->getData()));
            }

            // Executes the halide stage
            schedules[procType]->realize(im_ios, params);

            // Assigns the output mat to its DataRegion
            dr_ios[dr_ios.size()-1]->setData(im_ios[im_ios.size()-1]);

        }
    }* currentTask = new _Task(this->schedules, dr_ios, this->params);

    this->executeTask(currentTask);
}

RTF::Internal::AutoStage* RTF::AutoStage::genStage(SysEnv& sysEnv) {
    // Generate current stage if it was not already generated
    if (generatedStage == NULL) {
        generatedStage = new Internal::AutoStage(rts, 
            out_shape, schedules, params);
        // generatedStage->instantiateRegionTemplates();
    } else {
        return generatedStage;
    }

    // Generate dependent stages
    for (AutoStage* dep : deps) {
        // Generate the dependent stage while also adding it as a
        // dependency for the current stage
        generatedStage->addDependency(dep->genStage(sysEnv)->getId());
    }
    sysEnv.executeComponent(generatedStage);
}

// First implementation only has one stage
void RTF::AutoStage::execute(int argc, char** argv) {
    std::string shd_lib_name = "libautostage.so";

    cout << "[AutoStage::Execute] starting sys" << endl;
    SysEnv sysEnv;
    sysEnv.startupSystem(argc, argv, shd_lib_name);

    // Generate all stages and send them for execution
    this->genStage(sysEnv);

    // Startup execution is this is the final stage of the pipeline
    cout << "[AutoStage::Execute] executing pipeline" << endl;
    // if (last_stage) {
        sysEnv.startupExecution();
    // }
    sysEnv.finalizeSystem();
}

// Create the component factory
PipelineComponentBase* componentFactoryAutoStage() {
    return new RTF::Internal::AutoStage();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister(
    "AutoStage", &componentFactoryAutoStage);
