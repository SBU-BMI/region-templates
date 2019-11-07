/*
 * SysEnv.cpp
 *
 *  Created on: Feb 15, 2012
 *      Author: george
 */

#include <mpi.h>
#include "SysEnv.h"

#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <cstring>


#include "Util.h"
#include "Manager.h"
#include "Worker.h"
#include <stdlib.h>

SysEnv::SysEnv() {
	this->manager = NULL;

}

SysEnv::~SysEnv() {
	if(this->manager != NULL){
		delete this->manager;
	}
}

// initialize MPI
MPI_Comm init_mpi(int argc, char **argv, int &size, int &rank, std::string &hostname) {
    int initialized;
    MPI_Initialized(&initialized);
    if (!initialized)
    	MPI_Init(&argc, &argv);

    char *temp = new char[256];
    gethostname(temp, 255);
    hostname.assign(temp);
    delete [] temp;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    return MPI_COMM_WORLD;
}

Manager *SysEnv::getManager() const
{
    return manager;
}

void SysEnv::setManager(Manager *manager)
{
    this->manager = manager;
}
void SysEnv::parseInputArguments(int argc, char**argv){
	// Init parameters to create Worker to default values
	cpus = 1;
	gpus = 0;
	managerQueueType = ExecEngineConstants::FCFS_QUEUE;
	windowSize=-1;
	// FCFS
	policy = ExecEngineConstants::FCFS_QUEUE;
	dataLocalityAware = false;
	prefetching = false;

	cacheOnRead = false;
	componentDataAwareSchedule = false;

	std::cout << "Parse arguments system: argc: "<< argc <<std::endl;
	for(int i = 0; i < argc; i++){
		std::cout << "argv["<<i<<"]="<<argv[i]<< std::endl;
	}

	// Used for parameters parsing
	string policyArgumentValue;

	// Used for parameters parsing
	for(int i = 0; i < argc; i++){
		if(argv[i][0] == '-'){
			switch (argv[i][1])
			{
			case 'c':
				cpus = atoi(argv[i+1]);
				std::cout << "cpu option"<<std::endl;
				break;

			case 'd':
				dataLocalityAware = true;
				break;

			case 'p':
				prefetching = true;
				break;

			case 'g':
				gpus = atoi(argv[i+1]);
				break;

			case 'w':
				windowSize = atoi(argv[i+1]);
				break;

			case 's':
				policyArgumentValue = argv[i+1];
				if( !policyArgumentValue.compare("priority")){
					policy = ExecEngineConstants::PRIORITY_QUEUE;
				}else if(policyArgumentValue.compare("fcfs")){
					std::cout << "Unknow policy: "<< policyArgumentValue << ". Using FCFS."<<std::endl;
				}
				break;
			case 'r':
				cacheOnRead = true;
				break;
			case 'x':
				componentDataAwareSchedule = true;
				break;
			case 'h':
				managerQueueType = ExecEngineConstants::HALIDE_TARGET_QUEUE;
			default:
				break;

			}
		}
	}

	if(windowSize == -1){
		windowSize = cpus + gpus;
	}

}
int SysEnv::startupSystem(int argc, char **argv, std::string componentsLibName){
	// set up mpi
	int rank, size, worker_size, manager_rank;
	std::string hostname;

//	std::cout << "Before input args parse" << std::endl;
	parseInputArguments(argc, argv);
//	std::cout << "after input args parse" << std::endl;

	MPI_Comm comm_world = init_mpi(argc, argv, size, rank, hostname);

//	if(rank == 0){
//
////		        int npapp = this->comm_world.Get_size() -1;
//	int npapp = 1;
// //     std::cout << " DataSpaces clients: "<< npapp << std::endl;
//        dspaces_init(npapp, 1);
////        std::cout <<  " after DataSpaces init" << std::endl;
//	}
//


	std::cout << "Using: cpus="<<this->cpus<< " gpus="<< this->gpus << " policy="<< this->policy<< " dataLocalityAware?="<< dataLocalityAware<<std::endl;
	if (size == 1) {
		printf("ERROR: this must run with 2 or more MPI processes. 1 for the Manager and 1(+) for Worker(s)\n");
		MPI_Finalize();
		exit(1);
		return -4;
	}

	// initialize the worker comm object
	worker_size = size - 1;
	manager_rank = size - 1;


	uint64_t t1 = 0, t2 = 0;
	t1 = Util::ClockGetTime();

	// decide based on rank of worker which way to process
	if (rank == manager_rank) {
		// Create the manager process information
		this->setManager(new Manager(comm_world, manager_rank, worker_size, componentDataAwareSchedule, managerQueueType, cpus, gpus));

		// Check whether all Worker have successfully initialized their execution
		this->getManager()->checkConfiguration();

		// Return the Manager control flow the the user code, that
		// will presumably instantiate the pipeline for execution
		return 0;

	} else {
		// Create one worker object for each Worker process. The Worker objects follows
		// the singleton pattern and, as such, only the existing instance is retrieved
		// through the getInstance class
		Worker* localWorker = Worker::getInstance(manager_rank, rank, windowSize, cpus, gpus, policy, dataLocalityAware, prefetching, cacheOnRead);//new Worker(comm_world, manager_rank, rank, windowSize, cpus, gpus, policy, dataLocalityAware, prefetching );
		localWorker->setCommWorld(comm_world);

		// Initialize the name of the library containing the Pipeline components
		localWorker->setComponentLibName(componentsLibName);

		// This is the main computation loop, where the Worker
		// will keep asking for tasks and executing them.
		localWorker->workerProcess();

		// Delete Worker structures
		delete localWorker;
	}

	// Shake hands and finalize MPI
	MPI_Barrier(comm_world);
	MPI_Finalize();
	exit(0);

}

int SysEnv::startupExecution(){
	uint64_t t1 = 0, t2 = 0;
	printf("Manager StartupExecution");
	t1 = Util::ClockGetTime();
	std::cout << "MANAGER: startup time: "<< t1 << std::endl;

	this->getManager()->manager_process();

	t2 = Util::ClockGetTime();

	std::cout <<"MANAGER "<< this->getManager()->getManagerRank() <<": FINISHED in "<< t2 - t1 << " us" <<std::endl;
	return 1;
}

int SysEnv::finalizeSystem()
{
	return this->getManager()->finalizeExecution();
}

int SysEnv::executeComponent(PipelineComponentBase* compInstance) {
	this->getManager()->insertComponentInstance(compInstance);
	return 0;
}



