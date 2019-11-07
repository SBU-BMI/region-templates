/*
 * Manager.h
 *
 *  Created on: Feb 15, 2012
 *      Author: george
 */

#ifndef MANAGER_H_
#define MANAGER_H_

#include <mpi.h>
#include <vector>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#include "MessageTag.h"
#include "DataPack.h"
#include "PipelineComponentBase.h"
#include "./regiontemplates/RTPipelineComponentBase.h"

//class RTPipelineComponentBase;
class PipelineComponentBase;

class Manager {
private:
	// Identifies whether it is the first time the Manager is executing the "manager_process" function.
	bool FirstExecutionRound;

	// Number of Worker processes
	int worker_size;

	// Ranking of this manager within the communication group
	int manager_rank;

	// component data aware schedule
	bool componentDataAwareSchedule;

	// it stores for each worker, which component would reuse
	// data taken into account components finished.
	std::vector<std::vector<int> >compToSchedDataAwareSchedule;

	// Components with dependencies solved, meaning ready to execute
	TasksQueue* componentsToExecute;

	// Whether this manager uses the halide task queue
	bool halideQueue;

	// Mechanism that tracks dependencies among component instances
	TrackDependencies* componentDependencies;

	// Components that were sent for execution and did not finish yet
	map<int, PipelineComponentBase *> activeComponents;

	int insertActiveComponent(PipelineComponentBase* pc);

	// Results sent back from components (component id , result data)
	map<int, char *> componentsResultData;

	void insertComponentResultData(int id, char *data);

	// this retrieves a component and deletes it from the dependency graph
	PipelineComponentBase* retrieveActiveComponent(int id);

	// this operation only returns a component pointer, but it is not deleted.
	PipelineComponentBase* getActiveComponent(int id);

	int getActiveComponentsSize();

	// MPI structure defining the communication group information
	MPI_Comm comm_world;

	void sendComponentInfoToWorker(int worker_id, PipelineComponentBase *pc);

	void sendDRInfoToWorkerForGlobalStorage(int worker_id, std::string rtName, std::string rtId, DataRegion *dr);

	int resolveDependencies(PipelineComponentBase *pc);
	std::list<Task *> getDependentTasks(PipelineComponentBase *pc);

	bool isFirstExecutionRound() const;
	void setFirstExecutionRound(bool firstExecutionRound);

	friend class SysEnv;
	friend class PipelineComponentBase;

public:
	Manager(const MPI_Comm comm_world, const int manager_rank, const int worker_size, const bool componentDataAwareSchedule=false, const int queueType=ExecEngineConstants::FCFS_QUEUE, const int cpuThreads=1, const int gpuThreads=0);
	virtual ~Manager();

	void manager_process();
	int finalizeExecution();
	void checkConfiguration();

    MPI_Comm getCommWorld() const;
    int getManagerRank() const;
    int getWorkerSize() const;
    void setWorkerSize(int worker_size);

    int insertComponentInstance(PipelineComponentBase * compInstance);
    char *getComponentResultData(int id);
    bool eraseResultData(int id);

};

#endif /* MANAGER_H_ */
