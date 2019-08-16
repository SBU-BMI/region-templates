#include "AutoStage.h"

RTF::AutoStage::AutoStage(const std::vector<int>& out_shape,
                      const std::vector<ASInputs<>>& ios,
                      Target_t this_target,
                      HalGen* halGenFun) : out_shape(out_shape), ios(ios), 
                      this_target(this_target),
                      halGenFun(halGenFun) {
    this->setComponentName("???");
}

RTF::AutoStage::~AutoStage() {
    sysEnv.finalizeSystem();
}

    // First implementation only has one stage
void RTF::AutoStage::execute(int argc, char** argv) {
    sysEnv.startupSystem(argc, argv, "???.so");

    // for each dep of this {
    //     // This is done instead of executeComponent() for enforcing
    //     // that the dependencies are created before the actual dependent
    //     // stage (i.e., this)
    //     dep.execute(); 
    // }

    // Executes the current stage after dependencies
    sysEnv.executeComponent(this);

    // Startup execution is this is the final stage of the pipeline
    // if (last_stage) {
        sysEnv.startupExecution();
    // }
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
