/*
 * ThreadPool.cpp
 *
 *  Created on: Aug 18, 2011
 *      Author: george
 */

#include "ThreadPool.h"


// GPU functions called to initialize device.
//void warmUp(int device);
//void *cudaMemAllocWrapper(int dataSize);
//void cudaFreeMemWrapper(void *data_ptr);

void *callThread(void *arg){
	ThreadPool *tp = (ThreadPool *)((threadData*) arg)->threadPoolPtr;
	int procType = (int)((threadData*) arg)->procType;
	int tid = (int)((threadData*) arg)->tid;

	// If threads is managing GPU, than init adequate device
	if(procType == 2){
		printf("WarmUP: GPU id = %d\n", tid);
//		warmUp(tid);

		cv::gpu::setDevice(2);
//		cv::gpu::setDevice(tid);
//		cv::gpu::setDevice(tid);
/*		if(tid==1){
			std::cout << "GPU thread: "<< tid << " using GPU:"<< tid+1<<std::endl;
			cv::gpu::setDevice(tid+1);
		}else{
			std::cout << "GPU thread: "<< tid << " using GPU:"<< tid<<std::endl;
			cv::gpu::setDevice(tid);
		}*/
		int cpuId=tid;

		cpu_set_t cpu_info;
		CPU_ZERO(&cpu_info);
		if(tid==2){
			cpuId=11;
		}
		CPU_SET(cpuId, &cpu_info);


		if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_info) == -1) {
			printf("Error: sched_getaffinity\n");
		}

	}else{
		int cpuId=tid+2;

////		cpu_set_t cpu_info;
////		CPU_ZERO(&cpu_info);
////		CPU_SET(cpuId, &cpu_info);
////
////
////		if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_info) == -1) {
////			printf("Error: sched_getaffinity\n");
////		}



	}

	tp->processTasks(procType, tid);
	free(arg);
	pthread_exit(NULL);
}

ThreadPool::ThreadPool(TasksQueue *tasksQueue, ExecutionEngine *execEngine) {
	// init thread pool with user provided task queue
	this->tasksQueue = tasksQueue;
	this->execEngine = execEngine;

	CPUWorkerThreads = NULL;
	GPUWorkerThreads = NULL;

	pthread_mutex_init(&initExecutionMutex, NULL);
	pthread_mutex_lock(&initExecutionMutex);

	pthread_mutex_init(&createdThreads, NULL);
	pthread_mutex_lock(&createdThreads);

	numGPUThreads = 0;
	numCPUThreads = 0;
//	gpuTempDataSize = 0;
	firstToFinish=true;
	execDone=false;
}

ThreadPool::~ThreadPool() {
	execDone=true;
	this->finishExecWaitEnd();

	if(CPUWorkerThreads != NULL){
		free(CPUWorkerThreads);
	}

	if(GPUWorkerThreads != NULL){
		free(GPUWorkerThreads);
	}
	pthread_mutex_destroy(&initExecutionMutex);

	float loadImbalance = 0.0;
	if(numCPUThreads+numGPUThreads > 1){
		// calculate time in microseconds
		double tS = firstToFinishTime.tv_sec*1000000 + (firstToFinishTime.tv_usec);
		double tE = lastToFinishTime.tv_sec*1000000  + (lastToFinishTime.tv_usec);
		loadImbalance = (tE - tS)/1000000.0;
	}
	printf("Load imbalance = %f\n", loadImbalance);

}

//void * ThreadPool::getGPUTempData(int tid){
//	//return this->gpuTempData[tid];
//}

bool ThreadPool::createThreadPool(int cpuThreads, int *cpuThreadsCoreMapping, int gpuThreads, int *gpuThreadsCoreMapping, bool dataLocalityAware, bool prefetching)
{

//	this->gpuTempDataSize = gpuTempDataSize;
	this->dataLocalityAware = dataLocalityAware;
	this->prefetching = prefetching;
	if(prefetching)
		std::cout << "Prefetching is on!"<<std::endl;

	// Create CPU threads.
	if(cpuThreads > 0){
		numCPUThreads = cpuThreads;
		CPUWorkerThreads = (pthread_t *) malloc(sizeof(pthread_t) * cpuThreads);

		for (int i = 0; i < cpuThreads; i++ ){
			threadData *arg = (threadData *) malloc(sizeof(threadData));
			arg->tid = i;
			arg->procType = ExecEngineConstants::CPU;
			arg->threadPoolPtr = this;
			int ret = pthread_create(&(CPUWorkerThreads[arg->tid]), NULL, callThread, (void *)arg);
			if (ret){
				printf("ERROR: Return code from pthread_create() is %d\n", ret);
				exit(-1);
			}

			// wait untill thead is created
			pthread_mutex_lock(&createdThreads);
		}
	}
	// Create CPU threads.
	if(gpuThreads > 0){
		numGPUThreads = gpuThreads;
		GPUWorkerThreads = (pthread_t *) malloc(sizeof(pthread_t) * gpuThreads);

		for (int i = 0; i < gpuThreads; i++ ){
			threadData *arg = (threadData *) malloc(sizeof(threadData));
			arg->tid = i;
			arg->procType = ExecEngineConstants::GPU;
			arg->threadPoolPtr = this;
			int ret = pthread_create(&(GPUWorkerThreads[arg->tid]), NULL, callThread, (void *)arg);
			if (ret){
				printf("ERROR: Return code from pthread_create() is %d\n", ret);
				exit(-1);
			}
//			gpuTempData.push_back(NULL);
			// wait untill thead is created
			pthread_mutex_lock(&createdThreads);

		}
	}

	return true;
}


void ThreadPool::initExecution()
{
	pthread_mutex_unlock(&initExecutionMutex);
}

void ThreadPool::enqueueUploadTaskParameters(Task* task, cv::gpu::Stream& stream) {
	for(int i = 0; i < task->getNumberArguments(); i++){
		task->getArgument(i)->upload(stream);
	}
}


void ThreadPool::downloadTaskOutputParameters(Task* task, cv::gpu::Stream& stream) {
	for(int i = 0; i < task->getNumberArguments(); i++){
		// Download only those parameters that are of type: output, or input_output.
		// Input type arguments can be deleted directly.
		if(task->getArgument(i)->getType() != ExecEngineConstants::INPUT){
			task->getArgument(i)->download(stream);
		}
	}
	stream.waitForCompletion();

	for(int i = 0; i < task->getNumberArguments(); i++){
		task->getArgument(i)->deleteGPUData();
	}

}



void ThreadPool::preassignmentSelectiveDownload(Task* task, Task* preAssigned, cv::gpu::Stream& stream) {
	vector<int> downloadingTasksIds;

	// Perform parameter matching, and download what will not be used
	// for all parameters of the current task
	for(int i = 0; i < task->getNumberArguments(); i++){
		// check if matches one parameter of the preAssigned task
		int j;
		for(j = 0; j < preAssigned->getNumberArguments(); j++){
			// if matches, leave the loop
			if(task->getArgument(i)->getId() == preAssigned->getArgument(j)->getId() ){
				break;
			}
		}
		// Did not find a matching, download
		if(j == preAssigned->getNumberArguments()){
			std::cout << "Parameters i = "<< i <<" did not match!" << std::endl;
			if(task->getArgument(i)->getType() != ExecEngineConstants::INPUT){
				std::cout << "	Type is not input = "<< task->getArgument(i)->getType() <<std::endl;
				task->getArgument(i)->download(stream);
				downloadingTasksIds.push_back(i);

			}else{
				std::cout << "	Type is input only = "<< task->getArgument(i)->getType() <<std::endl;
				task->getArgument(i)->deleteGPUData();
			}
		}
	}
	stream.waitForCompletion();
	// Now release GPU memory for arguments that we have downloaded
	for(int i = 0; i < downloadingTasksIds.size(); i++){
		task->getArgument(downloadingTasksIds[i])->deleteGPUData();
	}

}


void ThreadPool::processTasks(int procType, int tid)
{
	cv::gpu::Stream stream;

	// stream used to perform datat prefetching
	cv::gpu::Stream pStream;

	// Inform that this threads was created.
	//sem_post(&createdThreads);
	pthread_mutex_unlock(&createdThreads);

	printf("procType:%d  tid:%d waiting init of execution\n", procType, tid);
	pthread_mutex_lock(&initExecutionMutex);
	pthread_mutex_unlock(&initExecutionMutex);

	Task* curTask = NULL;
	Task* preAssigned = NULL;
	Task* prefetchTask = NULL;

	// ProcessTime example
	struct timeval startTime;
	struct timeval endTime;

	// Increment tasks semaphore to avoid the thread from get stuck when taking the prefeteched task
	if(this->prefetching ==  true){
		// Each GPU has one extra task available to compute.
		this->tasksQueue->releaseThreads(this->numGPUThreads);
	}

	while(true){

		curTask = this->tasksQueue->getTask(procType);
		if(curTask == NULL){

			if(this->execEngine->getCountTasksPending() != 0 || this->execDone == false){

				// Did not actually took a task, so increment the number of tasks processed
				this->tasksQueue->incrementTasksToProcess();

				// if, for any reason, could not return a new tasks a prefetchTask exist, compute the pre-fetched one
				if(prefetchTask != NULL){
					curTask = prefetchTask;
					prefetchTask = NULL;

					goto procPoint;
				}
				usleep(10000);

				// All tasks ready to execute are finished, but there still existing tasks pending due to unsolved dependencies
				continue;
			}


			printf("procType:%d  tid:%d Task NULL #t_pending=%d\n", procType, tid, this->execEngine->getCountTasksPending());
			break;
		}
		uploadPoint:


		procPoint:

		gettimeofday(&startTime, NULL);
		try{
			// if GPU, it is necessary to upload input data
			if(procType == ExecEngineConstants::GPU){

				enqueueUploadTaskParameters(curTask, stream);
				stream.waitForCompletion();
			}
			curTask->run(procType, tid);

			if(this->dataLocalityAware){
				if(procType ==  ExecEngineConstants::GPU){
					std::cout << "AFTER run: isDataLocalityAware!"<<std::endl;
					// 1) call pre-assignment.
					preAssigned = curTask->tryPreassignment();
					if(preAssigned != NULL){
						preassignmentSelectiveDownload(curTask, preAssigned, stream);

					}else{
						std::cout << "Preassignement NULL!!!"<<std::endl;
					}

				}
			}

			// there is not task preAssigned, so download everything until the next tasks is chosen for execution
			if(preAssigned == NULL){
				if(procType == ExecEngineConstants::GPU)
					downloadTaskOutputParameters(curTask, stream);
//				// If GPU, download output data residing in GPU
//				if(procType == ExecEngineConstants::GPU){
//					for(int i = 0; i < curTask->getNumberArguments(); i++){
//						// Donwload only those parameters that are of type: output, or
//						// input_output. Input type arguments can be deleted directly.
//						if(curTask->getArgument(i)->getType() != ExecEngineConstants::INPUT){
//							curTask->getArgument(i)->download(stream);
//						}
//					}
//					stream.waitForCompletion();
//					for(int i = 0; i < curTask->getNumberArguments(); i++){
//						curTask->getArgument(i)->deleteGPUData();
//					}
//				}
			}


		}catch(...){
			printf("ERROR in tasks execution. EXCEPTION");
		}

		gettimeofday(&endTime, NULL);

		// Resolve pending dependencies.

		// calculate time in microseconds
		double tS = startTime.tv_sec*1000000 + (startTime.tv_usec);
		double tE = endTime.tv_sec*1000000  + (endTime.tv_usec);
		printf("procType:%d  tid:%d Task speedup = %f procTime = %f\n", procType, tid, curTask->getSpeedup(), (tE-tS)/1000000);
		delete curTask;

		if(preAssigned != NULL){
			curTask = preAssigned;
			preAssigned = NULL;
			goto procPoint;
		}

	}

//	printf("Leaving procType:%d  tid:%d #t_pending=%d\n", procType, tid, this->execEngine->getCountTasksPending());
//	if(ExecEngineConstants::GPU == procType && gpuTempDataSize >0){
//		cudaFreeMemWrapper(gpuTempData[tid]);
//	}
	// I don't care about controlling concurrent access here. Whether two threads
	// enter this if, their resulting gettimeofday will be very similar if not the same.
	if(firstToFinish){
		firstToFinish = false;
		gettimeofday(&firstToFinishTime, NULL);
	}

	gettimeofday(&lastToFinishTime, NULL);
}

int ThreadPool::getGPUThreads()
{
	return numGPUThreads;
}



void ThreadPool::finishExecWaitEnd()
{
	// make sure to init the execution if was not done by the user.
	initExecution();

	for(int i= 0; i < numCPUThreads; i++){
		pthread_join(CPUWorkerThreads[i] , NULL);
	}
	for(int i= 0; i < numGPUThreads; i++){
		pthread_join(GPUWorkerThreads[i] , NULL);
	}

}


int ThreadPool::getCPUThreads()
{
	return numCPUThreads;
}





