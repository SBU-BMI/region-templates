/*
 * Worker.h
 *
 *  Created on: Feb 15, 2012
 *      Author: george
 */

#ifndef WORKER_H_
#define WORKER_H_

#include <mpi.h>
#include <string>
#include <dlfcn.h>
#include <assert.h>
#include "MessageTag.h"
#include "PipelineComponentBase.h"
#include "CallBackComponentExecution.h"
#include "DataPack.h"

#ifdef	WITH_DATA_SPACES
extern "C" {
#include "dataspaces.h"
}
#endif

#ifdef WITH_RT
#include "Cache.h"
#include "TaskMoveDR2Global.h"
class Cache;
#endif
class CallBackComponentExecution;

class Worker {

private:
	// This Workers rank
	int rank;

	// Rank assigned to the Manager process
	int manager_rank;

    // Communication group of this execution
    MPI_Comm comm_world;

	// Name of the library that implements the components executed by this worker
	std::string componentLibName;

	// List of components (Ids) that were completed the execution within this Worker. The
	// Resource Manager computing threads will insert items to this list using the
	// call back function that is executed when all tasks associated to a given component
	// have been successfully executed. The main Worker controlling threads will keep
	// checking whether elements were added to this list, and notify the Manger when so.
	list<char*> computedComponents;
	list<int> computedComponentsDataSize;
	list<char*> computedComponentsData;

	// Stores a reference to each pipeline component instantiated in the
	// system, which has not yet finished their execution. Pipeline component
	// unique id is used as the map key.
	map<int, PipelineComponentBase*> activeComponentsRef;

	// Used to serialize access to list of components already computed
	pthread_mutex_t activeComponentsRefLoc;

    // Counter of currently active cpu/gpu thread instances 
    int availableCpuThreads;
    int availableGpuThreads;

    // Allocate a thread given a task's target list, prioritizing gpu allocation
    void allocateThreadType(std::list<int> targets);
    void deallocateThreadType(int target);

	void storeActiveComponentRef(PipelineComponentBase* pc);
	void deleteActiveComponentRef(PipelineComponentBase* pc);

	// Used to serialize access to list of components already computed
	pthread_mutex_t computedComponentsLock;

	// Number of active component instances within this Worker.
	int activeComponentInstances;

	// Max number of active components maintained locally to this Worker.
	int maxActiveComponentInstances;

	// setter for the counter described above
    void setActiveComponentInstances(int activeComponentInstances);

	// Resource manager that executes component tasks assigned to this worker
	ExecutionEngine *resourceManager;

	// Set and Get the current resource manager
    ExecutionEngine *getResourceManager() const;
    void setResourceManager(ExecutionEngine *resourceManager);

	// Function that loads the library holding the components
    bool loadComponentsLibrary();

    // Receive information about this execution, load library, and prepare to start computation
    void configureExecutionEnvironment();

    // Receive data describing a pipeline component instantiation that should be executed
    PipelineComponentBase *receiveComponentInfoFromManager();

    void receiveCacheInfoFromManager();

    // Send message to Manager notifying it about the end of components, if there are any in the list of computedComponents
    void notifyComponentsCompleted();

    // This is a singleton class. Only a single instance exists per process
    Worker(const int manager_rank, const int rank, const int max_active_components, const int CPUCores, const int GPUs, const int schedType, const bool dataLocalityAware, const bool prefetching, bool cacheOnRead = false);

    // Other variables used to guarantee that it is a singleton
    static bool instanceFlag;
    static Worker *singleWorker;

    // This class is responsible for deleting a given component PipelineComponentBase
    // from the map of active ones
    friend class CallBackComponentExecution;

#ifdef WITH_RT
    Cache *cache;
	Cache* getCache() const;
	void setCache(Cache* cache);
#endif

#ifdef PROFILING
    long long workerPrepareTime;
#endif

public:

    static Worker* getInstance(const int manager_rank=0, const int rank=0, const int max_active_components=1, const int CPUCores=1, const int GPUs=0, const int schedType=ExecEngineConstants::FCFS_QUEUE, const bool dataLocalityAware=false, const bool prefetching=false, bool cacheOnRead = false);
    virtual ~Worker();

    // Main loop that keeps receiving component instances and executing them
    void workerProcess();

    const MPI_Comm getCommWorld() const;

    // Simply return the values of the manager rank and this workers rank
    int getManagerRank() const;
    int getRank() const;

    // Retrieve the name of the library that implements the components used by this worker
    std::string getComponentLibName() const;

    // Set the same library name
    void setComponentLibName(std::string componentLibName);

    // Add a given component id to the output list of already computed component instances
    void addComputedComponent(char *, int dataSize=0, char *data=NULL);

    // Try to return a component id from the output list. This will return the
    // component id, or -1 whether there are not components into the output list
   // int getComputedComponent();

    // Retrieve number of components in the output list
    int getComputedComponentSize();

    // Increments a counter responsible to hold the number of component instances active with the current Worker
    int incrementActiveComponentInstances();

    // Decrements the same counter previously described
    int decrementActiveComponentInstances();

    // Simple getter for the active component counter
    int getActiveComponentInstances() const;
    int getMaxActiveComponentInstances() const;
    void setMaxActiveComponentInstances(int maxActiveComponentInstances);

    // retrieve a pointer to the current component from it id
	PipelineComponentBase* retrieveActiveComponentRef(int id);

    void printHello(){
    	std::cout << "Hey! componentLibName: "<< this->getComponentLibName() << std::endl;
    };
	void setCommWorld(MPI_Comm commWorld) {
        this->comm_world = commWorld;
    }
};


#endif /* WORKER_H_ */
