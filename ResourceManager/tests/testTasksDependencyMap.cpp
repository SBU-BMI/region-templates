/*
 *
 *  Created on: Feb 7, 2011
 *      Author: george
 */

#include "TasksQueue.h"
#include <stdio.h>

int main(){

	map<int, list<Task *> > dependencyMap;
	map<int, list<Task *> >::iterator dependencyMapIt;

	Task *ts = new Task();
	ts->printDependencies();
	int taskId = ts->getId();
	list<Task *> l;
	dependencyMap.insert(std::pair<int, list<Task *> >(ts->getId(), l));

	l.push_back(ts);

	dependencyMapIt = dependencyMap.find(taskId);

	if(dependencyMapIt != dependencyMap.end()){
		std::cout << "l.size()="<<l.size()<<std::endl;
		std::cout << "id="<< dependencyMapIt->first << " listSize="<< dependencyMapIt->second.size()<<std::endl;
		dependencyMapIt->second.push_back(ts);
		std::cout << "id="<< dependencyMapIt->first << " listSize="<< dependencyMapIt->second.size()<<std::endl;
		std::cout << "depId = "<< dependencyMapIt->second.front()->getId() << std::endl;
		dependencyMap.erase(dependencyMapIt);
		std::cout << "dependencyMap.size()="<<dependencyMap.size()<<std::endl;
	}
	delete ts;




//	printf( "FCFS QUEUE\n" );
//	TasksQueueFCFS tasksQueue;
//
//	Task *auxTask = new Task();
//	auxTask->setSpeedup(ExecEngineConstants::GPU, 2.0);
//	tasksQueue.insertTask(auxTask);
//
//	auxTask = new Task();
//	auxTask->setSpeedup(ExecEngineConstants::GPU, 5.0);
//	tasksQueue.insertTask(auxTask);
//
//	auxTask = new Task();
//	auxTask->setSpeedup(ExecEngineConstants::GPU, 4.0);
//	tasksQueue.insertTask(auxTask);
//
//	auxTask = new Task();
//	auxTask->setSpeedup(ExecEngineConstants::GPU, 3.0);
//	tasksQueue.insertTask(auxTask);
//	tasksQueue.releaseThreads(1);
//
//	auxTask=NULL;
//	do{
//
//		auxTask = tasksQueue.getTask(ExecEngineConstants::CPU);
//		if(auxTask != NULL){
//			printf("Task speedup = %f\n", auxTask->getSpeedup());
//			delete auxTask;
//		}
//	}while(auxTask!= NULL);
//
//	printf("PRIORITY QUEUE\n");
//
//	TasksQueuePriority tasksQueueP;
//
//	auxTask = new Task();
//	auxTask->setSpeedup(ExecEngineConstants::GPU, 2.0);
//	tasksQueueP.insertTask(auxTask);
//
//	auxTask = new Task();
//	auxTask->setSpeedup(ExecEngineConstants::GPU, 5.0);
//	tasksQueueP.insertTask(auxTask);
//
//	auxTask = new Task();
//	auxTask->setSpeedup(ExecEngineConstants::GPU, 4.0);
//	tasksQueueP.insertTask(auxTask);
//
//	auxTask = new Task();
//	auxTask->setSpeedup(ExecEngineConstants::GPU, 3.0);
//	tasksQueueP.insertTask(auxTask);
//	tasksQueueP.releaseThreads(1);
//
//	auxTask=NULL;
//	do{
//
//		auxTask = tasksQueueP.getTask(ExecEngineConstants::CPU);
//		if(auxTask != NULL){
//			printf("Task speedup = %f\n", auxTask->getSpeedup());
//			delete auxTask;
//		}
//	}while(auxTask!= NULL);



	return 0;
}
