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
		fflush(stdout);
//		warmUp(tid);

#ifdef	WITH_CUDA

//		cv::gpu::setDevice(2);
		cv::gpu::setDevice(tid);

	//	cv::gpu::setDevice(tid);
//		if(tid==1){
//			std::cout << "GPU thread: "<< tid << " using GPU:"<< tid+1<<std::endl;
//			cv::gpu::setDevice(tid+1);
//		}else{
//			std::cout << "GPU thread: "<< tid << " using GPU:"<< tid<<std::endl;
//			cv::gpu::setDevice(tid);
//		}
		int cpuId=tid;


//		cpu_set_t cpu_info;
//		CPU_ZERO(&cpu_info);
//		if(tid==2){
//			cpuId=11;
//		}
//		CPU_SET(cpuId, &cpu_info);
//
//		if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_info) == -1) {
//			printf("Error: sched_getaffinity\n");
//		}
		// warmup gpu
		cv::Mat A;
		A = cv::Mat::zeros(3,3,CV_32F);
		cv::gpu::GpuMat A_g(A);
		A_g.release();
#endif
		
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

void ThreadPool::enqueueDownloadTaskParameters(Task* task, cv::gpu::Stream& stream) {
	for(int i = 0; i < task->getNumberArguments(); i++){
		task->getArgument(i)->download(stream);
	}
}

void ThreadPool::deleteOutputParameters(Task* task) {
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
//			std::cout << "Parameters i = "<< i <<" did not match!" << std::endl;
			if(task->getArgument(i)->getType() != ExecEngineConstants::INPUT){
//				std::cout << "	Type is not input = "<< task->getArgument(i)->getType() <<std::endl;
				task->getArgument(i)->download(stream);
				downloadingTasksIds.push_back(i);

			}else{
//				std::cout << "	Type is input only = "<< task->getArgument(i)->getType() <<std::endl;
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

#ifdef WITH_CUDA
	cv::gpu::Stream *stream;

	// stream used to perform data prefetching
	cv::gpu::Stream *pStream;

	// stream used to perform asynchronous download of data
	cv::gpu::Stream *dStream;

	if(procType == ExecEngineConstants::GPU){
		try{
			stream = new cv::gpu::Stream();
			pStream = new cv::gpu::Stream();
			dStream = new cv::gpu::Stream();
		}catch(...){
			printf("ERROR creating stream. EXCEPTION\n");
		}

	}
#endif
	// Inform that this threads was created.
	//sem_post(&createdThreads);
	pthread_mutex_unlock(&createdThreads);

	printf("procType:%d  tid:%d waiting init of execution\n", procType, tid);
	pthread_mutex_lock(&initExecutionMutex);
	pthread_mutex_unlock(&initExecutionMutex);

	Task* curTask = NULL;
	Task* preAssigned = NULL;
	Task* prefetchTask = NULL;
	Task* downloadingTask = NULL;
	vector<int> downloadArgIds;

	// ProcessTime example
	struct timeval startTime;
	struct timeval endTime;

	// Increment tasks semaphore to avoid the thread from get stuck when taking the prefetched task
	if(this->prefetching == true && procType == ExecEngineConstants::GPU ){
		// Each GPU has one extra task available to compute.
		this->tasksQueue->releaseThreads(2);
	}

	while(true){

		curTask = this->tasksQueue->getTask(procType);

		afterGetTask:

		// Did not succeed in returning a task for computation
		if(curTask == NULL){
			// if It is not the end of execution
			if(this->execEngine->getCountTasksPending() != 0 || this->execDone == false){

				// Did not actually took a task, so increment the number of tasks processed
				this->tasksQueue->incrementTasksToProcess();

#ifdef WITH_CUDA
				// if, for any reason, could not return a new tasks a prefetchTask exist, compute the pre-fetched one
				if(prefetchTask != NULL){
					// update current task with pointer to prefetched task, and set prefetch to NULL
					curTask = prefetchTask;
					prefetchTask = NULL;

					// make sure that prefetched upload is completed.
					pStream->waitForCompletion();
//					std::cout << "Proc prefetched data, not enqueueing another"<<std::endl;

					goto procPoint;
				}else{
					// If there is nothing to compute, check whether there is an asynchronous
					// download in course, and solve it. This may potentially create other tasks
					if(downloadingTask != NULL){
//						std::cout<< "No other tasks available... resolve asyncDownload"<<std::endl;
						// wait until download is complete
						dStream->waitForCompletion();

						// release GPU memory space used by those downloaded parameters
						deleteOutputParameters(downloadingTask);

						// delete task being download, what will solve its dependencies
						delete downloadingTask;

						// set task to NULL, meaning that no other task is being downloaded
						downloadingTask = NULL;
						continue;
					}
				}
#endif
				usleep(10000);

				// All tasks ready to execute are finished, but there still existing tasks pending due unsolved dependencies
				continue;
			}else{
				printf("procType:%d  tid:%d Task NULL #t_pending=%d\n", procType, tid, this->execEngine->getCountTasksPending());
				break;
			}
		}else{
			// It this tasks was canceled, resolve its dependencies and try to get another task
			if(curTask->getStatus() != ExecEngineConstants::ACTIVE ){

				if(curTask->getTaskType() == ExecEngineConstants::TRANSACTION_TASK){
					curTask->run(procType, tid);
				} 
				
				delete curTask;
				continue;
			}

#ifdef WITH_CUDA
			// if prefetching is active
			if(this->prefetching){
				// Process task previously prefetched, and dispatch upload of current task
				if(prefetchTask != NULL){
					// wait until data of the prefetched task is ready. We hope it is already in place
					pStream->waitForCompletion();

					Task* temp = curTask;
					curTask = prefetchTask;
					prefetchTask = temp;
					enqueueUploadTaskParameters(prefetchTask, *pStream);
//					std::cout << "Proc prefetched data, enqueue another"<<std::endl;

				}else{
					// If curTask is not NULL, and prefetch Taks is NULL there is a chance for performing
					// the prefetching. This is the point where it startup
					if(prefetchTask == NULL && procType == ExecEngineConstants::GPU){
						// set the curTask to the prefetching
						prefetchTask = curTask;
						enqueueUploadTaskParameters(prefetchTask, *pStream);
						curTask = NULL;
//						std::cout << "Enqueue prefetched data"<<std::endl;

						// Try to get another task
						continue;
					}
				}
			}else{
				if(procType == ExecEngineConstants::GPU){
					// Not using prefetching, enqueue parameters upload and wait for completion
					enqueueUploadTaskParameters(curTask, *stream);
					stream->waitForCompletion();
				}
			}
#endif
		}

		procPoint:

		gettimeofday(&startTime, NULL);

		double tSComp = startTime.tv_sec*1000000 + (startTime.tv_usec);

//		printf("StartTime:%f\n",(tSComp)/1000000);
		try{
			
			std::cout << "Executing, task.id: "<< curTask->getId() << std::endl;
			curTask->run(procType, tid);

			if(curTask->getStatus() != ExecEngineConstants::ACTIVE){

#ifdef WITH_CUDA
				if(procType == ExecEngineConstants::GPU){
					// release GPU memory space used by this tasks
					deleteOutputParameters(curTask);
				}
#endif
				delete curTask;

				continue;
			}

#ifdef WITH_CUDA
			if(this->dataLocalityAware){
				if(procType ==  ExecEngineConstants::GPU){
//					std::cout << "AFTER run: isDataLocalityAware!"<<std::endl;
					// 1) call pre-assignment.
					preAssigned = curTask->tryPreassignment();
				}
			}
			// check whether there is an asynchronous download in course, and solve it
			if(downloadingTask != NULL){
//				std::cout<< "After processing... resolve asyncDownload"<<std::endl;
				// wait until download is complete
				dStream->waitForCompletion();

				// release GPU memory space used by those downloaded parameters
				deleteOutputParameters(downloadingTask);

				// delete task being download, what will solve its dependencies
				delete downloadingTask;

				// set task to NULL, meaning that no other task is being downloaded
				downloadingTask = NULL;
			}

			// there is not task preAssigned, so download everything until the next tasks is chosen for execution
			if(preAssigned == NULL){
				if(procType == ExecEngineConstants::GPU){
					// if prefetching is not active, perform synchronous download
					if(!prefetching){
						downloadTaskOutputParameters(curTask, *stream);
				
						delete curTask;
						curTask = NULL;
					}else{
//						downloadTaskOutputParameters(curTask, stream);
//						delete curTask;
//						std::cout<< "preAssigned Failed... start asyncDownload"<<std::endl;
						// do the asynchronous one.
						enqueueDownloadTaskParameters(curTask, *dStream);
						downloadingTask = curTask;
						curTask = NULL;
					}
				}
			}else{
				if(procType == ExecEngineConstants::GPU){
					preassignmentSelectiveDownload(curTask, preAssigned, *stream); // download only what is not reused by preassigned task
					delete curTask;
					curTask = preAssigned;
					preAssigned = NULL;
					goto afterGetTask;
				}
			}
#endif
		}catch(...){
			printf("ERROR in tasks execution. EXCEPTION\n");
			curTask->setStatus(ExecEngineConstants::CANCELED);

#ifdef WITH_CUDA
			try{
	      			deleteOutputParameters(curTask);
			}catch(...){
				printf("Delete failed\n");
			}
#endif
			// TODO: delete any available GPU data, and continue (what if the CPU is cancelled)
			delete curTask;
			continue;
		}

		gettimeofday(&endTime, NULL);



		// calculate time in microseconds
		double tS = startTime.tv_sec*1000000 + (startTime.tv_usec);
		double tE = endTime.tv_sec*1000000  + (endTime.tv_usec);
		printf("procType:%d taskId:%d  tid:%d procTime = %f\n", procType, curTask->getId(), tid,  (tE-tS)/1000000);

		// October 04, 2013. Commenting out line bellow to work with GPUs without compiling w/ cuda/gpu suppport
	//	if(procType == ExecEngineConstants::CPU){
			try{	
				delete curTask;
			}catch(...){
				printf("ERROR DELETE\n");
			}
//		}
		
		
		
//		if(curTask != NULL)
//			delete curTask;
//
//		if(preAssigned != NULL){
//			curTask = preAssigned;
//			preAssigned = NULL;
//			goto afterGetTask;
//		}

	}

	printf("Leaving procType:%d  tid:%d #t_pending=%d\n", procType, tid, this->execEngine->getCountTasksPending());
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





