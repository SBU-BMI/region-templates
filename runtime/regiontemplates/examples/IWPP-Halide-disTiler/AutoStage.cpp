#include "AutoStage.h"

// namespace RTF {

// enum Target_t {
//     CPU,
//     GPU
// }

// class AutoStage : public RTPipelineComponentBase {
//     std::vector<Halide::Argument> args;
//     std::vector<ASInputs> inputs;
//     std::map<Target_t, Halide::Func> schedules;
//     std::vector<int> out_shape; // Rows at 0, cols at 
//     Target_t this_target;

//     AutoStage(const std::vector<Halide::Argument>& args, 
//               const std::vector<ASInputs>& inputs,
//               const std::map<Target_t, Halide::Func>& schedules,
//               const std::vector<int>& out_shape,
//               Target_t this_target) 
//             : args(args), inputs(inputs), schedules(schedules), 
//               out_shape(out_shape), this_target(this_target) {

//         this->setComponentName("???");
//     }

//     ~AutoStage() {
//         sysEnv.finalizeSystem();
//     }

//     // First implementation only has one stage
//     void execute() {
//         sysEnv.startupSystem(argc, argv, "???.so");

//         // for each dep of this {
//         //     // This is done instead of executeComponent() for enforcing
//         //     // that the dependencies are created before the actual dependent
//         //     // stage (i.e., this)
//         //     dep.execute(); 
//         // }

//         // Executes the current stage after dependencies
//         sysEnv.executeComponent(this);

//         // Startup execution is this is the final stage of the pipeline
//         // if (last_stage) {
//             sysEnv.startupExecution();
//         // }
//     }

//     int run() {
//         // Anonymous class for implementing the current stage's task
//         class _Task : Task {
//             int out_cols, out_rows;
//             std::vector<Halide::Argument> args;
//             std::vector<ASInputs> inputs;
//             std::map<Target_t, Halide::Func> schedules;
//             Target_t this_target;

//             _Task(int out_cols, int out_rows) 
//                 : out_cols(out_cols), out_rows(out_rows), 
//                   args(args), inputs(inputs), schedules(schedules)
//                   this_target(this_target) {}

//             bool run(int procType=ExecEngineConstants::CPU, int tid=0) {
//                 // Output buffer must be pre-allocated for the halide pipeline
//                 cv::Mat cvOut(this->out_rows, this->out_cols, CV_8U);
//                 Halide::Buffer<DATA_T> hOut(
//                     cvOut.data, this->out_cols, this->out_rows);

//                 // Connect inputs from the halide pipeline
//                 for (int i=0; i<this->args.size(); i++) {
//                     switch (this->inputs[i].type) {
//                         case ASInputs::DataRegion:
//                             DataRegion* dr = this->inputs[i].getDR(this);
//                             cv::Mat cvIn = dr->getData();
//                             Halide::Buffer<DATA_T> hIn(cvIn->data, 
//                                 cvIn->cols, cvIn->rows);
//                             if (this->this_target == GPU)
//                                 hIn.set_host_dirty();
//                             this->args[i].set(hIn);
//                             break;
//                         case ASInputs::Param:
//                             this->args[i].set(this->inputs[i].getParam());
//                             break;
//                     }
//                 }
                
//                 // Realizes the halide pipeline
//                 this->schedules[this->this_target].realize(hOut);

//                 // Write the output on the RTF data hierarchy
//                 if (this->this_target == GPU) {
//                     hOut.copy_to_host();
//                 }
//                 this->drOut->setData(cvOut);
//             }
//         } currentTask(out_shape[0], out_shape[1], this->args, 
//             this->inputs, this->schedules, this_target);

//         this->executeTask(currentTask);
//     }

//     // distribute();
// }


// } // namespace RTF
