/*
 * Worker.cpp
 *
 *  Created on: Feb 15, 2012
 *      Author: george
 */

#include "Worker.h"

// initialize singleton related control variables
bool Worker::instanceFlag = false;
Worker *Worker::singleWorker = NULL;

Worker::Worker(const int manager_rank, const int rank,
               const int max_active_components, int CPUCores, int GPUs,
               const bool withHybridWorkarround, const int schedType,
               const bool dataLocalityAware, const bool prefetching,
               const bool cacheOnRead) {
    this->manager_rank = manager_rank;
    this->rank = rank;
    this->setMaxActiveComponentInstances(max_active_components);
    this->setActiveComponentInstances(0);
    // Create a local Resource Manager

    // DELETE THIS LATER
    if (withHybridWorkarround) {
        if (rank == 0) {
            std::cout
                << "====== Doing the n workers 1 CPU, n-1 GPU thread pools: "
                   "NEW CPU WORKER"
                << std::endl;
            GPUs = 0;
        } else {
            std::cout
                << "====== Doing the n workers 1 CPU, n-1 GPU thread pools: "
                   "NEW GPU WORKER"
                << std::endl;
            CPUCores = 0;
        }
    }

    this->setResourceManager(new ExecutionEngine(
        CPUCores, GPUs, rank, schedType, dataLocalityAware, prefetching));

    // Computing threads startup consuming tasks
    this->getResourceManager()->startupExecution();

    this->availableCpuThreads = CPUCores;
    this->availableGpuThreads = GPUs;

#ifdef WITH_RT
    this->cache = new Cache("rtconf.xml");
    this->cache->setWorkerId(this->rank);
    this->cache->setCacheOnRead(cacheOnRead);
    std::cout << "[Worker][W" << rank
              << "] CacheOnRead: " << this->cache->isCacheOnRead() << std::endl;
#endif

#ifdef PROFILING
    this->workerPrepareTime = 0;
#endif

    pthread_mutex_init(&this->computedComponentsLock, NULL);
    pthread_mutex_init(&this->activeComponentsRefLoc, NULL);

    this->comm_world = MPI_COMM_WORLD;
}

Worker *Worker::getInstance(const int manager_rank, const int rank,
                            const int max_active_components, const int CPUCores,
                            const int GPUs, const bool withHybridWorkarround,
                            const int schedType, const bool dataLocalityAware,
                            const bool prefetching, const bool cacheOnRead) {
    if (!instanceFlag) {
        singleWorker =
            new Worker(manager_rank, rank, max_active_components, CPUCores,
                       GPUs, withHybridWorkarround, schedType,
                       dataLocalityAware, prefetching, cacheOnRead);
        instanceFlag = true;
        return singleWorker;
    } else {
        return singleWorker;
    }
}

Worker::~Worker() {
    // No more task will be assigned for execution. Waits until all currently
    // assigned have finished.
    this->getResourceManager()->endExecution();

    // Delete the Resource Manager
    delete this->getResourceManager();
#ifdef WITH_RT
    delete this->cache;
#endif

    pthread_mutex_destroy(&this->computedComponentsLock);
    pthread_mutex_destroy(&this->activeComponentsRefLoc);
}

const MPI_Comm Worker::getCommWorld() const { return comm_world; }

int Worker::getManagerRank() const { return manager_rank; }

std::string Worker::getComponentLibName() const { return componentLibName; }

void Worker::setComponentLibName(std::string componentLibName) {
    this->componentLibName = componentLibName;
}

bool Worker::loadComponentsLibrary() {
    bool retValue = true;
    char *error = NULL;
#ifdef DEBUG
    std::cout << "Load ComponentsLibrary(). Libname: "
              << this->getComponentLibName() << std::endl;
#endif
    // try load lib in local directory, so we need 2 strigns,
    // the first with ./, em the second whithout
    std::string libNameLocal;
    libNameLocal.append("./");
    libNameLocal.append(this->getComponentLibName());

    // get the library handler
    void *componentLibHandler = NULL;
    if (((componentLibHandler =
              dlopen(this->getComponentLibName().c_str(), RTLD_NOW)) == NULL) &&
        ((componentLibHandler = dlopen(libNameLocal.c_str(), RTLD_NOW)) ==
         NULL)) {
        fprintf(stderr, "Could not Components %s library, %s\n",
                this->getComponentLibName().c_str(), dlerror());
        dlclose(componentLibHandler);
        std::cout << "Could not load library components: "
                  << this->getComponentLibName() << std::endl;
        retValue = false;
    } else {
        std::cout << "Library Components successfully load" << std::endl;
    }

    return retValue;
}

void Worker::configureExecutionEnvironment() {
    // Load component library
    bool successConf = this->loadComponentsLibrary();

    // Sent configuration status to the Manager: so far, it only means
    // that the components library was correctly initialized
    MPI_Send(&successConf, 1, MPI_C_BOOL, this->getManagerRank(),
             MessageTag::TAG_CONTROL, this->comm_world);

    // Receive global initialization result from the Manager
    MPI_Bcast(&successConf, 1, MPI_C_BOOL, this->getManagerRank(),
              this->comm_world);

    // Finalize the execution if the initialization failed.
    if (successConf == false) {
        std::cout << "Quitting. Initialization failed, Worker rank:"
                  << this->getRank() << std::endl;
        MPI_Finalize();
        exit(1);
    }
}

PipelineComponentBase *Worker::receiveComponentInfoFromManager() {
    PipelineComponentBase *pc = NULL;
    MPI_Status status;
    int message_size;

    // Probe for incoming message from Manager
    MPI_Probe(this->getManagerRank(), (int)MessageTag::TAG_METADATA,
              this->comm_world, &status);

    // Check the size of the input message
    MPI_Get_count(&status, MPI_CHAR, &message_size);

    char *msg = new char[message_size];

    //  std::cout << "Msg size="<<message_size<<std::endl;

    // get data from manager
    MPI_Recv(msg, message_size, MPI_CHAR, this->getManagerRank(),
             MessageTag::TAG_METADATA, this->comm_world, MPI_STATUS_IGNORE);

    // Unpack the name of the component to instantiate it and deserialize
    // message
    int comp_name_size = ((int *)msg)[1];
    char *comp_name = new char[comp_name_size + 1];
    comp_name[comp_name_size] = '\0';
    memcpy(comp_name, msg + (2 * sizeof(int)), comp_name_size * sizeof(char));
    //	std::cout << "CompName="<< comp_name <<std::endl;

    pc =
        PipelineComponentBase::ComponentFactory::getCompoentFromName(comp_name);
    if (pc != NULL) {
        pc->deserialize(msg);
        pc->setLocation(PipelineComponentBase::WORKER_SIDE);
    } else {
        std::cout << "Error reading component name=" << comp_name << std::endl;
    }
    delete[] msg;
    delete[] comp_name;
    return pc;
}

void Worker::receiveCacheInfoFromManager() {
    MPI_Status status;
    int message_size;

    // Probe for incoming message from Manager
    MPI_Probe(this->getManagerRank(), (int)MessageTag::TAG_CACHE_INFO,
              this->comm_world, &status);

    // Check the size of the input message
    MPI_Get_count(&status, MPI_CHAR, &message_size);

    char *msg = new char[message_size];

    // get data from manager
    MPI_Recv(msg, message_size, MPI_CHAR, this->getManagerRank(),
             MessageTag::TAG_CACHE_INFO, this->comm_world, MPI_STATUS_IGNORE);

    int deserialized_bytes = 0;
    std::string rtName, rtId, drName, drId, inputFileName;
    int timestamp, version;

    // unpack information about the DR that should be moved to global storage
    DataPack::unpack(msg, deserialized_bytes, rtName);
    DataPack::unpack(msg, deserialized_bytes, rtId);
    DataPack::unpack(msg, deserialized_bytes, drName);
    DataPack::unpack(msg, deserialized_bytes, drId);
    DataPack::unpack(msg, deserialized_bytes, inputFileName);
    DataPack::unpack(msg, deserialized_bytes, timestamp);
    DataPack::unpack(msg, deserialized_bytes, version);

#ifdef DEBUG
    std::cout << "move2Global: " << rtName << " " << rtId << " " << drName
              << " " << drId << " " << timestamp << " " << version << std::endl;
#endif

    //#ifdef WITH_RT
    // TaskMoveDR2Global *tMoveDr = new TaskMoveDR2Global(this->cache,
    // rtName,rtId, drName, drId, timestamp, version);
    // this->resourceManager->insertTask(tMoveDr);
    this->cache->move2Global(rtName, rtId, drName, drId, inputFileName,
                             timestamp, version);
    //#endif

    delete[] msg;
}

ExecutionEngine *Worker::getResourceManager() const {
    return this->resourceManager;
}

void Worker::setResourceManager(ExecutionEngine *resourceManager) {
    this->resourceManager = resourceManager;
}

int Worker::getRank() const { return rank; }

// #define WORKER_VERBOSE
// Allocate a thread given a task's target list
void Worker::allocateThreadType(std::list<int> targets) {
#ifdef WORKER_VERBOSE
    std::cout << "[Worker] allocating on worker with " << targets.size()
              << std::endl;
#endif

    for (int target : targets) {
        if (target == ExecEngineConstants::CPU &&
            this->availableCpuThreads > 0) {
            this->availableCpuThreads--;
#ifdef WORKER_VERBOSE
            std::cout << "[Worker] allocating cpu on worker" << std::endl;
#endif
            return;
        }
        if (target == ExecEngineConstants::GPU &&
            this->availableGpuThreads > 0) {
            this->availableGpuThreads--;
#ifdef WORKER_VERBOSE
            std::cout << "[Worker] allocating gpu on worker" << std::endl;
#endif
            return;
        }
    }
#ifdef WORKER_VERBOSE
    std::cout << "[Worker] Didn't allocate anything: avCPU="
              << this->availableCpuThreads
              << ", avGPU=" << this->availableGpuThreads << std::endl;
#endif
    exit(-1);
}

void Worker::deallocateThreadType(int target) {
    if (target == ExecEngineConstants::CPU)
        this->availableCpuThreads++;
    else if (target == ExecEngineConstants::GPU)
        this->availableGpuThreads++;
}

void Worker::workerProcess() {
    // Init data spaces
    // Number of DataSpaces clients equals to number of workers
    // Load Components implemented by the current application, and
    // check if all workers were correctly initialized.
    this->configureExecutionEnvironment();

    //	std::cout << "Worker: " << this->getRank() << ", before ready Barrier"
    //<< std::endl;

    int comm_size;
    MPI_Comm_size(this->comm_world, &comm_size);
#ifdef WITH_DATA_SPACES
    int npapp = comm_size - 1;
    std::cout << " DataSpaces clients: " << npapp << std::endl;
    //  dspaces_init(npapp, 1);
    std::cout << " after DataSpaces init" << std::endl;
#endif

    // Wait until the Manager startups the execution
    MPI_Barrier(this->comm_world);
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    std::cout << "[Worker][W" << this->getRank()
              << "] ready. Hostname: " << hostname
              << ", number of workers: " << comm_size << std::endl;

    // Flag that control the execution loop, and is updated from messages sent
    // by the Manager
    char flag = MessageTag::MANAGER_READY;
    while (flag != MessageTag::MANAGER_FINISHED &&
           flag != MessageTag::MANAGER_ERROR) {
#ifdef PROFILING
        long workerPrepareT1 = Util::ClockGetTime();
#endif

        // tell the manager - ready
        // Also states the type of thread (CPU or GPU or ...) this is
        char ready[3];
        ready[0] = MessageTag::WORKER_READY;
        ready[1] = this->availableCpuThreads;
        ready[2] = this->availableGpuThreads;
        // tell the manager - ready
        MPI_Send(&ready, 3, MPI_CHAR, this->getManagerRank(),
                 MessageTag::TAG_CONTROL, this->comm_world);

        // get the manager status
        MPI_Recv(&flag, 1, MPI_CHAR, this->getManagerRank(),
                 MessageTag::TAG_CONTROL, this->comm_world, MPI_STATUS_IGNORE);

        //	std::cout << "Worker: "<< this->getRank()<<" flag: "
        //<<(int)flag<<std::endl;

        switch (flag) {
            case MessageTag::MANAGER_READY: {
                PipelineComponentBase *pc =
                    this->receiveComponentInfoFromManager();
#ifdef DEBUG
                std::cout << "maxActive: "
                          << this->getMaxActiveComponentInstances()
                          << " activeComps: "
                          << this->getActiveComponentInstances()
                          << " #resTasks:" << std::endl;
#endif

                if (pc != NULL) {
                    // One more component instance was received and is being
                    // dispatched for execution
                    this->incrementActiveComponentInstances();

                    this->storeActiveComponentRef(pc);

                    // Associated local resource manager to the received
                    // component
                    pc->setResourceManager(this->getResourceManager());

#ifdef WITH_RT
                    pc->setCache(this->getCache());

                    // Decrements the counter of free threads for the current
                    // target ready[1]
                    this->allocateThreadType(pc->getTaskTargets());
#endif

                    // Create a Transaction tasks that relates subtasks created
                    // by this component to itself. When all subtasks have
                    // finished the call back is executed, destroyed and the
                    // component is destroyed
                    int ioTaskId = pc->createIOTask();

                    CallBackComponentExecution *callBackTask =
                        new CallBackComponentExecution(pc, this);
                    callBackTask->addTaskTarget(pc->getTaskTargets());
                    if (ioTaskId != -1) {
                        callBackTask->addDependency(ioTaskId);
                    }
#ifdef DEBUG
                    std::cout << "Created IO task. Id = " << ioTaskId
                              << std::endl;
                    std::cout << "CallBackTask id: " << callBackTask->getId()
                              << std::endl;
#endif
                    // Start transaction. All tasks created within the execution
                    // engine will be associated to this one
                    this->getResourceManager()->startTransaction(callBackTask);

                    // Execute component function that instantiates tasks within
                    // the execution engine
                    pc->run();

                    // Stop transaction: defines the end of the transaction
                    // associated to the current component
                    this->getResourceManager()->endTransaction();

                    // Dispatch transaction task for execution
                    this->getResourceManager()->insertTask(callBackTask);

                } else {
                    std::cout
                        << "[Worker] Error: Failed to load PipelineComponent!"
                        << std::endl;
                }

#ifdef PROFILING
                long workerPrepareT2 = Util::ClockGetTime();
                this->workerPrepareTime += workerPrepareT2 - workerPrepareT1;
#endif

                break;
            }
            case MessageTag::MANAGER_FINISHED: {
                std::cout << "[Worker][W" << this->getRank()
                          << "] Manager finished execution." << std::endl;
                break;
            }
            case MessageTag::MANAGER_WORK_QUEUE_EMPTY: {
                // Wait until ask for some work again
                usleep(1000);

                break;
            }
            case MessageTag::CACHE_MOVE_DR_TO_GLOBAL_STORAGE: {
                this->receiveCacheInfoFromManager();

                break;
            }
            default: {
                std::cout << "Manager:" << __FILE__ << ":" << __LINE__
                          << ". Unknown message type:" << (int)flag
                          << std::endl;
                break;
            }
        }

        this->notifyComponentsCompleted();
        // Okay. Reached the limit of component instances that could be
        // concurrently active at this Worker. Wait until at least one instance
        // is finished
        while (this->getMaxActiveComponentInstances() ==
               this->getActiveComponentInstances()) {
            // Wait some time before trying again. Avoiding busy wait here.
            usleep(10000);

            // check whether a component has finished, and notify the Manager
            // about that
            this->notifyComponentsCompleted();
        }
    }

#ifdef PROFILING
    std::cout << "[PROFILING][WORKER_PREP_TIME][W" << this->rank << "] "
              << this->workerPrepareTime << std::endl;
#endif

    // std::cout<< "ENDDDD EXECUTION = "<< this->getComputedComponentSize()
    // <<std::endl;
    // Assert that all tasks queued for execution are completed before leaving
    this->getResourceManager()->endExecution();

    // std::cout<< "ComputedComponents = "<< this->getComputedComponentSize()
    // <<std::endl;
    // Notify Manager about components finished: Those which were in the queue
    // for execution, and were computed during the endExecution call to above
    this->notifyComponentsCompleted();
    // std::cout<< "ComputedComponents = "<< this->getComputedComponentSize()
    // <<std::endl;
#ifdef WITH_DATA_SPACES
    // finalize connection with data spaces
//	dspaces_finalize();
#endif
}

void Worker::addComputedComponent(char *compData, int dataSize, char *data) {
    // Get list lock before accessing it
    pthread_mutex_lock(&this->computedComponentsLock);

    this->computedComponents.push_back(compData);
    if (dataSize > 0) {
        this->computedComponentsDataSize.push_back(dataSize);
        this->computedComponentsData.push_back(data);
    }

    // Release list lock
    pthread_mutex_unlock(&this->computedComponentsLock);
}

int Worker::getComputedComponentSize() {
    int computedComponentSize = 0;

    // Lock access to computed components list
    pthread_mutex_lock(&this->computedComponentsLock);

    computedComponentSize = this->computedComponents.size();

    // Release list lock
    pthread_mutex_unlock(&this->computedComponentsLock);
    return computedComponentSize;
}

void Worker::notifyComponentsCompleted() {
    if (this->getComputedComponentSize() > 0) {
        // Lock access to computed components list
        pthread_mutex_lock(&this->computedComponentsLock);

        // Number of components I need to pack
        int number_components = this->computedComponents.size();

        // Message format
        // |Mesg Type (char) | Number of Components to Pack (int) | the proper
        // components data (id, resultSize, result) | number of components with
        // extra data
        int message_size = sizeof(char) + sizeof(int);

        // add space for components data
        list<char *>::iterator itListCompCore =
            this->computedComponents.begin();
        for (; itListCompCore != this->computedComponents.end();
             itListCompCore++) {
            // space to store id + resultSize + result
            message_size +=
                sizeof(int) + sizeof(int) +
                ((int *)(*itListCompCore))[1] *
                    sizeof(char);  // result size is stored after the component
                                   // id in the buffer.
#ifdef DEBUG
            std::cout << "Worker: calculating result data size. ::: "
                      << ((int *)(*itListCompCore))[1]
                      << " message size:: " << message_size << std::endl;
#endif
        }

        // number of components w/ extra data
        message_size += sizeof(int);

        // save space for the components data size
        list<int>::iterator itListComp =
            this->computedComponentsDataSize.begin();
        for (; itListComp != this->computedComponentsDataSize.end();
             itListComp++) {
            // space to store size of data (int) + space to store data itself
            message_size += sizeof(int) + *itListComp;
#ifdef DEBUG
            std::cout << "Worker: extra data size: " << *itListComp
                      << std::endl;
#endif
        }

#ifdef DEBUG
        std::cout << "\tEXTRA data size = " << message_size
                  << " #components = " << number_components << std::endl;
        std::cout << " computedComponents: " << number_components
                  << " componentsDataSize: "
                  << this->computedComponentsDataSize.size() << std::endl;
#endif
        if (this->computedComponentsDataSize.size() > 0) {
            assert(this->computedComponentsDataSize.size() ==
                   number_components);
        }
        // Allocate Message
        char *msg = new char[message_size];
        int packed_data_size = 0;

        // Pack type of message
        msg[0] = MessageTag::WORKER_TASKS_COMPLETED;
        packed_data_size += sizeof(char);

        // Pack number of components finished
        ((int *)(msg + packed_data_size))[0] = number_components;
        packed_data_size += sizeof(int);

        // Pointer to integer part of the message
        // int *int_data_ptr = (int*)(msg + sizeof(char));

        // Pack number of components finished
        // int_data_ptr[0] = number_components;

        // Pack ids, result, resultData of all components
        for (int i = 0; i < number_components; i++) {
            char *data = this->computedComponents.front();
            int bufferSize =
                sizeof(int) + sizeof(int) + ((int *)data)[1] * sizeof(char);
#ifdef DEBUG
            std::cout << "Worker: resultDataSize: " << ((int *)data)[1]
                      << std::endl;
#endif
            // Pack ith component
            memcpy(msg + packed_data_size, data, bufferSize);
            packed_data_size += bufferSize;

            // int_data_ptr[i+1] = this->computedComponents.front();

            // Removed ith component from the output list
            this->computedComponents.pop_front();
            this->decrementActiveComponentInstances();
            delete data;
        }

        // Store number of components with extra data
        ((int *)(msg + packed_data_size))[0] =
            this->computedComponentsDataSize.size();
#ifdef DEBUG
        std::cout << "Worker: components w/ extra data: "
                  << this->computedComponentsData.size()
                  << " packed_data_size: " << packed_data_size << std::endl;
#endif
        packed_data_size += sizeof(int);

        int numComponentDataElements = computedComponentsDataSize.size();
        for (int i = 0; i < numComponentDataElements; i++) {
            int sizeCompData = this->computedComponentsDataSize.front();
            char *compData = this->computedComponentsData.front();

#ifdef DEBUG
            std::cout << "\t Worker: sizeCompData: " << sizeCompData
                      << std::endl;
#endif
            memcpy(msg + packed_data_size, &sizeCompData, sizeof(int));
            packed_data_size += sizeof(int);

            memcpy(msg + packed_data_size, compData,
                   sizeof(char) * sizeCompData);
            packed_data_size += sizeof(char) * sizeCompData;

            delete[] compData;
            this->computedComponentsDataSize.pop_front();
            this->computedComponentsData.pop_front();
        }
#ifdef DEBUG
        std::cout << "Packed data: " << packed_data_size
                  << "messageSize: " << message_size << std::endl;
        std::cout << "number of components: "
                  << ((int *)(msg + sizeof(char)))[0] << std::endl;
        std::cout << "Worker: completed ID: "
                  << ((int *)(msg + sizeof(char)))[1] << std::endl;
#endif
        // Release list lock
        pthread_mutex_unlock(&this->computedComponentsLock);

        // Now we have a message (msg) that is ready to sent to the Master
        MPI_Send(msg, message_size, MPI_CHAR, this->getManagerRank(),
                 MessageTag::TAG_CONTROL, this->comm_world);

        // Remove message
        delete[] msg;
    }
}

int Worker::getMaxActiveComponentInstances() const {
    return maxActiveComponentInstances;
}

void Worker::setMaxActiveComponentInstances(int maxActiveComponentInstances) {
    assert(maxActiveComponentInstances > 0);
    this->maxActiveComponentInstances = maxActiveComponentInstances;
}

int Worker::getActiveComponentInstances() const {
    return activeComponentInstances;
}

void Worker::setActiveComponentInstances(int activeComponentInstances) {
    this->activeComponentInstances = activeComponentInstances;
}

/*int Worker::getComputedComponent()
{
        int componentId = -1;;
        // Get list lock before accessing it
        pthread_mutex_lock(&this->computedComponentsLock);

        if(this->computedComponents.size() > 0){
                componentId = this->computedComponents.front();
                this->computedComponents.pop_front();
        }

        // Release list lock
        pthread_mutex_unlock(&this->computedComponentsLock);
        return componentId;
}*/

int Worker::incrementActiveComponentInstances() {
    this->activeComponentInstances++;
    return this->activeComponentInstances;
}

int Worker::decrementActiveComponentInstances() {
    this->activeComponentInstances--;
    return this->activeComponentInstances;
}

void Worker::storeActiveComponentRef(PipelineComponentBase *pc) {
    pthread_mutex_lock(&this->activeComponentsRefLoc);
    this->activeComponentsRef.insert(std::make_pair(pc->getId(), pc));
    pthread_mutex_unlock(&this->activeComponentsRefLoc);
}

void Worker::deleteActiveComponentRef(PipelineComponentBase *pc) {
    std::map<int, PipelineComponentBase *>::iterator it;
    pthread_mutex_lock(&this->activeComponentsRefLoc);

    // find and delete component entry
    it = this->activeComponentsRef.find(pc->getId());
    this->activeComponentsRef.erase(it);

    pthread_mutex_unlock(&this->activeComponentsRefLoc);
}

#ifdef WITH_RT
Cache *Worker::getCache() const { return cache; }

void Worker::setCache(Cache *cache) { this->cache = cache; }
#endif

PipelineComponentBase *Worker::retrieveActiveComponentRef(int id) {
    PipelineComponentBase *ret = NULL;
    std::map<int, PipelineComponentBase *>::iterator it;
    pthread_mutex_lock(&this->activeComponentsRefLoc);

    // find and delete component entry
    it = this->activeComponentsRef.find(id);

    // If found, assign it to return variable
    if (it != this->activeComponentsRef.end()) {
        ret = it->second;
    }

    pthread_mutex_unlock(&this->activeComponentsRefLoc);
    return ret;
}
