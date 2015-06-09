#include "TaskCompute.h"

TaskCompute::TaskCompute(int cpu_time, int gpu_time, int mic_time, std::string name) {
	this->cpu_time = cpu_time;
	this->gpu_time = gpu_time;
	this->mic_time = mic_time;
	this->name = name;
	
	this->setSpeedup(ExecEngineConstants::CPU, 1.0);
	this->setSpeedup(ExecEngineConstants::GPU, (float)cpu_time/(float)gpu_time);
	this->setSpeedup(ExecEngineConstants::MIC, (float)cpu_time/(float)mic_time);
}

TaskCompute::~TaskCompute() {

}

bool TaskCompute::run(int procType, int tid)
{
	std::cout << "TaskName: " << name << " ProcType: "<< procType << " taskID: "<< this->getId()<<std::endl;
	
	switch(procType){
	case ExecEngineConstants::CPU:
		usleep(this->cpu_time);
		break;
	
	case ExecEngineConstants::GPU:
		usleep(this->gpu_time);
		break;
		
	case ExecEngineConstants::MIC:
		usleep(this->mic_time);
		break;
	default:
		std::cout << "Unknown procType: "<< procType << std::endl;
		break;
	}

	return true;
}



