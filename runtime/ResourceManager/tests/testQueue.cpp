/*
 * testQueue.cpp
 *
 *  Created on: Aug 17, 2011
 *      Author: george
 */


#include "TasksQueue.h"
#include <stdio.h>

int main(){

	std::cout << "FCFS QUEUE" << std::endl;
	TasksQueueFCFS tasksQueue(1,0);

	Task *auxTask = new Task();
	auxTask->setSpeedup(ExecEngineConstants::GPU, 2.0);
	tasksQueue.insertTask(auxTask);

	auxTask = new Task();
	auxTask->setSpeedup(ExecEngineConstants::GPU, 5.0);
	tasksQueue.insertTask(auxTask);

	auxTask = new Task();
	auxTask->setSpeedup(ExecEngineConstants::GPU, 4.0);
	tasksQueue.insertTask(auxTask);

	auxTask = new Task();
	auxTask->setSpeedup(ExecEngineConstants::GPU, 3.0);
	tasksQueue.insertTask(auxTask);
	tasksQueue.releaseThreads(1);

	auxTask=NULL;
	do{

		auxTask = tasksQueue.getTask(ExecEngineConstants::CPU);
		if(auxTask != NULL){
			std::cout << "Task speedup = " << auxTask->getSpeedup() << std::endl;
			delete auxTask;
		}
	}while(auxTask!= NULL);

	std::cout << "PRIORITY QUEUE" <<std::endl;

	TasksQueuePriority tasksQueueP(1,0);

	auxTask = new Task();
	auxTask->setSpeedup(ExecEngineConstants::GPU, 2.0);
	tasksQueueP.insertTask(auxTask);

	auxTask = new Task();
	auxTask->setSpeedup(ExecEngineConstants::GPU, 5.0);
	tasksQueueP.insertTask(auxTask);

	auxTask = new Task();
	auxTask->setSpeedup(ExecEngineConstants::GPU, 4.0);
	tasksQueueP.insertTask(auxTask);

	auxTask = new Task();
	auxTask->setSpeedup(ExecEngineConstants::GPU, 3.0);
	tasksQueueP.insertTask(auxTask);
	tasksQueueP.releaseThreads(1);

	auxTask=NULL;
	do{

		auxTask = tasksQueueP.getTask(ExecEngineConstants::CPU);
		if(auxTask != NULL){
			std::cout << "Task speedup = " << auxTask->getSpeedup() << std::endl;
			delete auxTask;
		}
	}while(auxTask!= NULL);



	return 0;
}
