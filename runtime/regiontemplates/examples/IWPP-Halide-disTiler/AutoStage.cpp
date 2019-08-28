#include "AutoStage.h"

#include <sys/stat.h>

template <typename T_PARAM>
int RTF::ASInputs<T_PARAM>::serialize(char* buff) {
    int serialized_bytes = 0;

    // packs the type of input
    memcpy(buff+serialized_bytes, &this->type, sizeof(Types));
    serialized_bytes += sizeof(Types);

    // packs the actual data
    switch (this->type) {
        case RT: {
            // packs the size of the string
            int rt_name_size = this->rt_name.size();
            memcpy(buff+serialized_bytes, &rt_name_size, sizeof(int));
            serialized_bytes += sizeof(int);

            // packs the string itself
            memcpy(buff+serialized_bytes, this->rt_name.c_str(), 
                sizeof(char)*rt_name_size);
            serialized_bytes += rt_name_size * sizeof(char);
            break;
        }
        case Param:
            // packs the parameter
            memcpy(buff+serialized_bytes, &this->param, sizeof(T_PARAM));
            serialized_bytes += sizeof(T_PARAM);
            break;
    }

    return serialized_bytes;
}

template <typename T_PARAM>
int RTF::ASInputs<T_PARAM>::deserialize(char* buff) {
    int deserialized_bytes = 0;

    // unpacks the type of input
    memcpy(&this->type, buff+deserialized_bytes, sizeof(Types));
    deserialized_bytes += sizeof(Types);

    switch (this->type) {
        case RT: {
            // unpacks the size of the string
            int rt_name_size;
            memcpy(&rt_name_size, buff+deserialized_bytes, sizeof(int));
            deserialized_bytes += sizeof(int);

            // unpacks the string itself
            char rt_name[rt_name_size+1];
            rt_name[rt_name_size] = '\0';
            memcpy(rt_name, buff+deserialized_bytes, sizeof(char)*rt_name_size);
            deserialized_bytes += rt_name_size * sizeof(char);
            this->rt_name = rt_name;
            break;
        }
        case Param:
            // unpacks the parameter
            memcpy(&this->param, buff+deserialized_bytes, sizeof(T_PARAM));
            deserialized_bytes += sizeof(T_PARAM);
            break;
    }

    return deserialized_bytes;
}

template <typename T_PARAM>
int RTF::ASInputs<T_PARAM>::size() {
    int in_size;
    switch (this->type) {
        case RT: {
            in_size = sizeof(int) + this->rt_name.size();
            break;
        }
        case Param:
            in_size = sizeof(T_PARAM);
            break;
    }

    return sizeof(Types) + in_size;
}

RTF::AutoStage::AutoStage() {
    this->setComponentName("AutoStage");
}

RTF::AutoStage::AutoStage(const std::vector<int>& out_shape,
                      const std::vector<ASInputs<>>& ios,
                      HalGen* halGenFun) : out_shape(out_shape), ios(ios), 
                      halGenFun(halGenFun) {
    this->setComponentName("AutoStage");
}

RTF::AutoStage::AutoStage(HalGen* halGenFun) : halGenFun(halGenFun) {
    this->setComponentName("AutoStage");
}

RTF::AutoStage::~AutoStage() {
    sysEnv.finalizeSystem();
}

    // First implementation only has one stage
void RTF::AutoStage::execute(int argc, char** argv) {
    std::string shd_lib_name = "autostagelib.so";
    // cout << "[AutoStage::Execute] creating shared lib if not found" << endl;
    // struct stat buffer;
    // if (stat (shd_lib_name.c_str(), &buffer) != 0) {
    //     system("");
    // }

    cout << "[AutoStage::Execute] starting sys" << endl;
    sysEnv.startupSystem(argc, argv, shd_lib_name);

    // for each dep of this {
    //     // This is done instead of executeComponent() for enforcing
    //     // that the dependencies are created before the actual dependent
    //     // stage (i.e., this)
    //     dep.execute(); 
    // }

    // Executes the current stage after dependencies
    cout << "[AutoStage::Execute] sending stage" << endl;
    sysEnv.executeComponent(this);

    // Startup execution is this is the final stage of the pipeline
    cout << "[AutoStage::Execute] executing pipeline" << endl;
    // if (last_stage) {
        sysEnv.startupExecution();
    // }
}

int RTF::AutoStage::serialize(char* buff) {
//  std::cout << "\t THIS IS RT_PIPELINE_COMPONENT:serialize" << std::endl;
    int serialized_bytes = RTPipelineComponentBase::serialize(buff);

    // packs the ios vector size
    int num_ios = this->ios.size();
    memcpy(buff+serialized_bytes, &num_ios, sizeof(int));
    serialized_bytes += sizeof(int);

    // packs the ios vector
    for (int i=0; i<this->ios.size(); i++) {
        serialized_bytes += this->ios[i].serialize(buff+serialized_bytes);
    }

     // packs the out_shape vector size
    int num_out_shape = this->out_shape.size();
    memcpy(buff+serialized_bytes, &num_out_shape, sizeof(int));
    serialized_bytes += sizeof(int);

    // packs the out_shape vector
    for (int i=0; i<this->out_shape.size(); i++) {
        memcpy(buff+serialized_bytes, &out_shape[i], sizeof(int));
        serialized_bytes += sizeof(int);
    }

    // memcpy(buff+serialized_bytes, &this_target, sizeof(RTF::Target_t));
    // serialized_bytes += sizeof(RTF::Target_t);

    return serialized_bytes;
}

int RTF::AutoStage::deserialize(char* buff) {
//  std::cout << "\t THIS IS RT_PIPELINE_COMPONENT:deserialize" << std::endl;
    int deserialized_bytes = RTPipelineComponentBase::deserialize(buff);

    // unpacks the ios vector size
    int num_ios;
    memcpy(&num_ios, buff+deserialized_bytes, sizeof(int));
    deserialized_bytes += sizeof(int);

    // unpacks the ios vector
    for(int i=0; i<num_ios; i++){
        ASInputs<> asi;
        deserialized_bytes += asi.deserialize(buff+deserialized_bytes);
        this->ios[i] = asi;
    }

    // unpacks the out_shape vector size
    int num_out_shape;
    memcpy(&num_out_shape, buff+deserialized_bytes, sizeof(int));
    deserialized_bytes += sizeof(int);

    for (int i=0; i<num_out_shape; i++) {
        int val;
        memcpy(buff+deserialized_bytes, &val, sizeof(int));
        deserialized_bytes += sizeof(int);
        this->out_shape[i] = val;
    }

    return deserialized_bytes;
}

int RTF::AutoStage::size() {
    // calculates the size of ios
    int ios_size = sizeof(int);
    for (int i=0; i<this->ios.size(); i++) {
        ios_size += ios[i].size();
    }

    // calculates the size of out_shape
    int out_shape_size = sizeof(int) * (this->out_shape.size() + 1);

    return RTPipelineComponentBase::size() + ios_size + out_shape_size;
}


RTF::AutoStage* RTF::AutoStage::clone() {
    AutoStage* retValue = new AutoStage();
    int size = this->size();
    char *buff = new char[size];
    this->serialize(buff);
    retValue->deserialize(buff);
    delete buff;
    return retValue;
}


int RTF::AutoStage::run() {
    // Anonymous class for implementing the current stage's task
    class _Task : public Task {
        std::vector<int> out_shape; // Rows at 0, cols at 1
        int out_cols, out_rows;
        std::vector<ASInputs<>> ios;
        Target_t this_target;
        RTPipelineComponentBase* pcb;
        HalGen* halGenFun;
    public:
        _Task(std::vector<int> out_shape, std::vector<ASInputs<>> ios,
            Target_t this_target, RTPipelineComponentBase* pcb,
            HalGen* halGenFun) : out_shape(out_shape), ios(ios),
              this_target(this_target), pcb(pcb), halGenFun(halGenFun) {
                this->out_rows = out_shape[0];
                this->out_cols = out_shape[1];
        }

        bool run(int procType=ExecEngineConstants::CPU, int tid=0) {
            // Output buffer must be pre-allocated for the halide pipeline
            cv::Mat cvOut(this->out_rows, this->out_cols, CV_8U);
            Halide::Buffer<DATA_T> hOut(
                cvOut.data, this->out_cols, this->out_rows);

            std::map<Target_t, Halide::Func> schedules;
            std::vector<HalImgParamOrParam<>> params;
            halGenFun->generate(schedules, params);

            // Connect inputs from the halide pipeline
            // Obs: params.size() + 1 = ios.size()
            // since the last param is the output
            for (int i=0; i<ios.size(); i++) {
                switch (this->ios[i].getType()) {
                    case ASInputs<>::RT: {
                        RegionTemplate* rt = this->ios[i].getRT(pcb);
                        DenseDataRegion2D* dr = dynamic_cast<DenseDataRegion2D*>(
                            rt->getDataRegion(rt->getName()));
                        cv::Mat cvIn = dr->getData();
                        Halide::Buffer<DATA_T> hIn(cvIn.data, 
                            cvIn.cols, cvIn.rows);
                        if (this->this_target == GPU)
                            hIn.set_host_dirty();
                        params[i].getImParam().set(hIn);
                        break;
                    }
                    case ASInputs<>::Param:
                        params[i].getParam().set(this->ios[i].getParam());
                        break;
                }
            }
            
            // Realizes the halide pipeline
            schedules[this->this_target].realize(hOut);

            // Write the output on the RTF data hierarchy
            if (this->this_target == GPU) {
                hOut.copy_to_host();
            }
            RegionTemplate* rtOut = this->ios[this->ios.size()-1].getRT(pcb);
            DenseDataRegion2D* drOut = dynamic_cast<DenseDataRegion2D*>(
                rtOut->getDataRegion(rtOut->getName()));
            drOut->setName(rtOut->getName());
            drOut->setData(cvOut);
            rtOut->insertDataRegion(drOut);
        }
    }* currentTask = new _Task(this->out_shape, this->ios, 
        this->this_target, this, this->halGenFun);

    this->executeTask(currentTask);
}

// Create the halide stage
static struct : RTF::HalGen {
    RTF::Target_t getTarget() {return RTF::CPU;}
    void generate(std::map<RTF::Target_t, Halide::Func>& schedules,
            std::vector<RTF::HalImgParamOrParam<>>& params) {
        Halide::ImageParam hI, hJ, hOut;
        Halide::Func halCpu;
        halCpu.define_extern("loopedIwppRecon2", {hI, hJ, hOut}, Halide::UInt(8), 2);
        schedules[RTF::CPU] = halCpu;
        params.emplace_back(hI);
        params.emplace_back(hJ);
        params.emplace_back(hOut);
    }
} stage1_hal;

// Create the component factory
PipelineComponentBase* componentFactoryAutoStage() {
    return new RTF::AutoStage(&stage1_hal);
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister(
    "AutoStage", &componentFactoryAutoStage);

