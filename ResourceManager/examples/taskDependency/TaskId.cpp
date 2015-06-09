#include "TaskId.h"

TaskId::TaskId() {
}

TaskId::~TaskId() {
	std::cout << "~TaskId" << std::endl;
}

bool TaskId::run(int procType, int tid)
{
	if(procType == ExecEngineConstants::CPU){
	//	sleep(5);
	}else{

		//sleep(5/this->getSpeedup(ExecEngineConstants::GPU));
	}
//	std::cout << "Task.id = "<< this->getId() << std::endl;
	this->printDependencies();
	return true;
}



