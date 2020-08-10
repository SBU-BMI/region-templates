#include "AutoStage.h"

RTF::Internal::AutoStage::AutoStage(std::vector<RegionTemplate*> rts,
                                    std::vector<int64_t> out_shape,
                                    std::map<Target_t, HalGen*> schedules,
                                    std::vector<ArgumentBase*> params,
                                    int tileId) {
    this->setComponentName("AutoStage");

    this->out_shape = out_shape;
    this->tileId = tileId;

    // Add parameters
    for (ArgumentBase* arg : params) this->addArgument(arg);

    // Gets the names of the registered stages
    for (std::pair<Target_t, HalGen*> s : schedules) {
        this->schedules[s.first] = s.second->getName();
        this->addTaskTarget(s.first);
    }

    std::string dr_name;

    // Populates the list of RTs names while also adding them to the RTPCB
    // Just the inputs here
    for (int i = 0; i < rts.size() - 1; i++) {
        dr_name =
            (tileId == -1 ? rts[i]->getName() : "t" + std::to_string(tileId));
        rts_names.emplace_back(rts[i]->getName());
        this->addRegionTemplateInstance(rts[i], rts[i]->getName());
        this->addInputOutputDataRegion(rts[i]->getName(), dr_name,
                                       RTPipelineComponentBase::INPUT);
    }

    // Add the output RT
    RegionTemplate* last_rt = rts[rts.size() - 1];
    dr_name =
        (tileId == -1 ? last_rt->getName() : "t" + std::to_string(tileId));
    rts_names.emplace_back(last_rt->getName());
    this->addRegionTemplateInstance(last_rt, last_rt->getName());
    this->addInputOutputDataRegion(last_rt->getName(), dr_name,
                                   RTPipelineComponentBase::OUTPUT);
};

void printTiled(cv::Mat tiledImg, std::list<cv::Rect_<int64_t>>& tiles,
                std::string name) {
    // For each tile of the current image
    cv::Mat img = tiledImg.clone();
    for (cv::Rect_<int64_t> tile : tiles) {
        // Adds tile rectangle region to tiled image
        cv::rectangle(img, cv::Point(tile.x, tile.y),
                      cv::Point(tile.x + tile.width, tile.y + tile.height),
                      (255, 255, 255), 5);
    }
    cv::imwrite(name + ".png", img);
}

void RTF::Internal::AutoStage::localTileDRs(
    std::list<cv::Rect_<int64_t>>& tiles,
    std::vector<std::vector<DenseDataRegion2D*>>& allTiles) {
    std::string drName = "t" + std::to_string(this->tileId);

    std::string rtId;
    RegionTemplate* rtCur;
    DenseDataRegion2D* drCur;

    int border = 0;
    TilerAlg_t denseTilingAlg = LIST_ALG_EXPECT;
    int nTiles = 1;

    // Gets RT for initial image
    rtId = this->rts_names[0];
    rtCur = this->getRegionTemplateInstance(rtId);

    // Sets cost functions
    int bgThr = 200;
    int erode_param = 12;
    int dilate_param = 4;
    BGMasker* bgm = new ThresholdBGMasker(bgThr, dilate_param, erode_param);
    CostFunction* cfunc =
        new ThresholdBGCostFunction(bgThr, dilate_param, erode_param);

    // Gets first DR for irregular tiling
    long stageTime0 = Util::ClockGetTime();
    drCur = dynamic_cast<DenseDataRegion2D*>(rtCur->getDataRegion(drName));
    cv::Mat cvInitial = drCur->getData();
    TiledMatCollection* preTiler = new BGPreTiledRTCollection(
        "local" + std::to_string(this->tileId),
        "local" + std::to_string(this->tileId), "", border, cfunc, bgm);
    TiledMatCollection* tCollImg =
        new IrregTiledRTCollection("local" + std::to_string(this->tileId),
                                   "local" + std::to_string(this->tileId), "",
                                   border, cfunc, bgm, denseTilingAlg, nTiles);

    long stageTime1 = Util::ClockGetTime();
    // std::cout << "[PROFILING_SINGLE][STAGE][TILER] " << cvInitial.rows << "x"
    //     << cvInitial.cols << " " << (stageTime1-stageTime0) << std::endl;

    // // Performs pre-tiling
    // preTiler->tileMat(cvInitial);

    // // Performs dense tiling
    // tiles =
    // dynamic_cast<BGPreTiledRTCollection*>(preTiler)->getDense().begin()->second;
    // std::list<cv::Rect_<int64_t>> bgTiles
    //     =
    //     dynamic_cast<BGPreTiledRTCollection*>(preTiler)->getBg().begin()->second;
    // if (tiles.size() > 0)
    //     tCollImg->tileMat(cvInitial, tiles);
    // tiles.insert(tiles.end(), bgTiles.begin(), bgTiles.end());
    // // printTiled(cvInitial, tiles, "denseTiled");
    // std::cout << "[AutoStage] tiled local " << cvInitial.cols << " x "
    //     << cvInitial.rows << " => " << tiles.size() << std::endl;
    tiles.emplace_back(
        cv::Rect_<int64_t>(0, 0, cvInitial.cols, cvInitial.rows));

    delete bgm;
    delete cfunc;
    // delete preTiler;
    // delete tCollImg;

    // allTiles[i][j] => i: internal tile id, j: RT id (i.e., each input image)
    std::vector<DenseDataRegion2D*> _init(this->rts_names.size());
    allTiles =
        std::vector<std::vector<DenseDataRegion2D*>>(tiles.size(), _init);

    // Performs tiling of all RTs for DRs into the same RT
    for (int i = 0; i < this->rts_names.size(); i++) {
#ifdef DEBUG
        std::cout << "=========[AutoStage] tiling RT: " << this->rts_names[i]
                  << std::endl;
#endif
        // Gets current RT
        rtId = this->rts_names[i];
        rtCur = this->getRegionTemplateInstance(rtId);

        // Gets current DR. Uses initial DR for i=0 to avoid opening it twice
        drName = this->tileId == -1 ? rtCur->getName() : drName;
        if (i > 0) {
            drCur =
                dynamic_cast<DenseDataRegion2D*>(rtCur->getDataRegion(drName));
        }
#ifdef DEBUG
        std::cout << "=========[AutoStage] tiling DR: " << drName << std::endl;
#endif

        // Tiles current DR
        int drNewId = 0;
        for (cv::Rect_<int64_t> tile : tiles) {
#ifdef DEBUG
            std::cout << "[AutoStage]\t tile: " << tile << std::endl;
#endif

            // Wraps cv tile inside a DR and adds it to the current RT
            DenseDataRegion2D* drNew = new DenseDataRegion2D();
            std::string drNewName = drName + "st" + to_string(drNewId);
            drNew->setName(drNewName);
            drNew->setId(rtCur->getName());
            drNew->setData(drCur->getData()(tile));
            rtCur->insertDataRegion(drNew);

            // Adds new DR to output container
            allTiles[drNewId++][i] = drNew;
#ifdef DEBUG
            std::cout << "[AutoStage]\t Created tile: " << drNewName
                      << std::endl;
#endif
        }
    }

    long stageTime9 = Util::ClockGetTime();
    // std::cout << "[PROFILING_SINGLE][STAGE][DR_GEN] " << cvInitial.rows <<
    // "x"
    //     << cvInitial.cols << " " << (stageTime9-stageTime1) << std::endl;
}

int RTF::Internal::AutoStage::run() {
// Assemble input/output cv::Mat list for execution
// Starts with the inputs
#ifdef DEBUG
    std::cout << "[Internal::AutoStage] running" << std::endl;
#endif
    std::string drName;

    long stageTime0 = Util::ClockGetTime();

    // Assign output mat to the correct output RT
    RegionTemplate* rtOut = this->getRegionTemplateInstance(
        this->rts_names[this->rts_names.size() - 1]);
    DenseDataRegion2D* drOut = new DenseDataRegion2D();

    drName = tileId == -1 ? rtOut->getName() : "t" + std::to_string(tileId);
    drOut->setName(drName);
    drOut->setId(rtOut->getName());
    drOut->setData(cv::Mat(this->out_shape[0], this->out_shape[1], CV_8U));
    rtOut->insertDataRegion(drOut);

    long stageTime1 = Util::ClockGetTime();
    // std::cout << "[PROFILING_SINGLE][STAGE][OUT_GEN] " << this->out_shape[0]
    //     << "x" << this->out_shape[1] << " "
    //     << (stageTime1-stageTime0) << std::endl;

#ifdef DEBUG
    std::cout << "[Internal::AutoStage] local tiling" << std::endl;
#endif

    // Perform local-worker tiling
    // tilesDRs[i][j] => i: internal tile id, j: RT id (i.e., each input image)
    std::vector<std::vector<DenseDataRegion2D*>> tilesDRs;
    std::list<cv::Rect_<int64_t>> tiles;
    localTileDRs(tiles, tilesDRs);

    long stageTime2 = Util::ClockGetTime();
    // std::cout << "[PROFILING_SINGLE][STAGE][TILE] " << this->out_shape[0] <<
    // "x"
    //     << this->out_shape[1] << " " << (stageTime2-stageTime1) << std::endl;

    // Assemble a schedule map with the local pointers for the halide functions
    std::map<Target_t, HalGen*> local_schedules;
    for (std::pair<Target_t, std::string> s : this->schedules) {
        local_schedules[s.first] = RTF::AutoStage::retrieveStage(s.second);
    }

#ifdef DEBUG
    std::cout << "[Internal::AutoStage] creating tasks" << std::endl;
#endif

    // Creates tasks for each tile
    std::list<int> tasksIds;
    for (int i = 0; i < tilesDRs.size(); i++) {
        // Anonymous class for implementing the current stage's task
        struct _Task : public Task {
            std::map<Target_t, HalGen*> schedules;
            std::vector<DenseDataRegion2D*> dr_ios;
            std::vector<ArgumentBase*> params;

            _Task(std::map<Target_t, HalGen*> schedules,
                  std::vector<DenseDataRegion2D*>& dr_ios,
                  std::vector<ArgumentBase*> params)
                : schedules(schedules), dr_ios(dr_ios), params(params){};

            bool run(int procType, int tid = 0) {
                long taskTime0 = Util::ClockGetTime();
// Generates the input/output list of cv::mat
#ifdef DEBUG
                std::cout << "[Internal::AutoStage::_Task] realizing "
                          << schedules.begin()->second->getName() << std::endl;
                std::cout << "[Internal::AutoStage::_Task] img size: "
                          << this->dr_ios[0]->getData().rows << " x "
                          << this->dr_ios[0]->getData().cols << std::endl;
#endif
                std::vector<cv::Mat> im_ios;
                bool aborted = false;
                for (int i = 0; i < this->dr_ios.size(); i++) {
                    im_ios.emplace_back(cv::Mat(this->dr_ios[i]->getData()));
                    if (this->dr_ios[i]->aborted()) {
                        aborted = true;
                        break;
                    }
                }

                // Executes the halide stage if not aborted
                if (!aborted)
                    aborted =
                        schedules[procType]->realize(im_ios, procType, params);
                if (aborted) {
                    std::string abortStr = "[Internal::AutoStage::_Task]";
                    abortStr += " Aborted exec of tiles with size ";
                    for (int i = 0; i < this->dr_ios.size(); i++) {
                        // set abort flag for all further data regions
                        this->dr_ios[i]->abort();

                        abortStr += to_string(this->dr_ios[i]->getData().rows);
                        abortStr += "x";
                        abortStr += to_string(this->dr_ios[i]->getData().cols);
                        abortStr += " ";
                    }
#ifdef DEBUG
                    std::cout << abortStr << std::endl;
#endif
                }

// Assigns the output mat to its DataRegion
#ifdef DEBUG
                std::cout << "[Internal::AutoStage::_Task] realized "
                          << std::endl;
#endif

                long taskTime1 = Util::ClockGetTime();
                // std::cout << "[PROFILING_SINGLE][TASK] "
                //     << this->dr_ios[0]->getData().rows << "x"
                //     << this->dr_ios[0]->getData().cols << " "
                //     << (taskTime1-taskTime0) << std::endl;
            }
        }* currentTask =
            new _Task(local_schedules, tilesDRs[i], this->getArguments());

        // Set targets for this task
        for (std::pair<Target_t, std::string> s : this->schedules) {
            currentTask->addTaskTarget(s.first);
        }

#ifdef DEBUG
        std::cout << "[Internal::AutoStage] sending task for execution"
                  << std::endl;
#endif
        this->executeTask(currentTask);
        tasksIds.emplace_back(currentTask->getId());
    }

    // Gets tiles for output DR
    int drOutIndex = tilesDRs[0].size() - 1;
    std::vector<DenseDataRegion2D*> outTileDRs(tiles.size());
    for (int i = 0; i < tiles.size(); i++)
        outTileDRs[i] = tilesDRs[i][drOutIndex];

    // Create a final task for merging the results
    struct _TaskMerge : public Task {
        DenseDataRegion2D* drOut;
        std::vector<DenseDataRegion2D*> tileDRs;
        std::list<cv::Rect_<int64_t>> tiles;

        _TaskMerge(DenseDataRegion2D* drOut,
                   std::vector<DenseDataRegion2D*> tileDRs,
                   std::list<cv::Rect_<int64_t>> tiles)
            : drOut(drOut), tileDRs(tileDRs), tiles(tiles){};

        bool run(int procType, int tid = 0) {
            long taskTime0 = Util::ClockGetTime();

            std::cout << "[_TaskMerge] merging " << this->tiles.size()
                      << " tiles" << std::endl;

            // Gets output DR mat
            cv::Mat cvOut = this->drOut->getData();

            // Add each tile to output mat
            int i = 0;
            for (cv::Rect_<int64_t> tile : this->tiles) {
                cv::Mat cvCur = this->tileDRs[i]->getData();
                if (!this->tileDRs[i]->aborted()) cvCur.copyTo(cvOut(tile));
                i++;
            }
            long taskTime1 = Util::ClockGetTime();

            // std::cout << "[PROFILING_SINGLE][MERGE] " << cvOut.rows
            //     << "x" << cvOut.cols << " " << (taskTime1-taskTime0)
            //     << std::endl;
        }
    }* currentTask = new _TaskMerge(drOut, outTileDRs, tiles);

    // Adds dependencies for merging task
    for (int tId : tasksIds) {
        currentTask->addDependency(tId);
    }

    // Scheduling for anything outside CPU do not make sense, since
    // we are only joining the images together.
    currentTask->addTaskTarget(this->schedules.begin()->first);
    // currentTask->addTaskTarget(ExecEngineConstants::CPU);

    this->executeTask(currentTask);

    long stageTime9 = Util::ClockGetTime();

    // std::cout << "[PROFILING_SINGLE][STAGE] " << this->out_shape[0] << "x"
    //     << this->out_shape[1] << " " << (stageTime9-stageTime0) << std::endl;
}

RTF::Internal::AutoStage* RTF::AutoStage::genStage(SysEnv& sysEnv) {
    // Generate current stage if it was not already generated
    if (this->generatedStage == NULL) {
        this->generatedStage = new Internal::AutoStage(
            rts, out_shape, schedules, params, this->tileId);
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
    if (RTF::AutoStage::stagesReg.find(stage->getName()) ==
        RTF::AutoStage::stagesReg.end()) {
        stagesReg[stage->getName()] = stage;
        return true;
    }
    return false;
}

RTF::HalGen* RTF::AutoStage::retrieveStage(std::string name) {
    return RTF::AutoStage::stagesReg.find(name)->second;
}
