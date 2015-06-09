#include "TaskRead.h"

TaskRead::TaskRead(std::string file_name) {
	this->file_name = file_name;
}

TaskRead::~TaskRead() {

}

bool TaskRead::run(int procType, int tid)
{
	// ProcessTime example
	struct timeval timeNow;
	
	std::cout << "TaskRead: " << this->file_name << " ProcType: "<< procType <<std::endl;

	gettimeofday(&timeNow, NULL);
	uint64_t t1 = timeNow.tv_sec*1000000 + (timeNow.tv_usec);
	
	cv::Mat input = cv::imread(this->file_name, -1);
	
	input.release();

	gettimeofday(&timeNow, NULL);
	uint64_t t2 = timeNow.tv_sec*1000000 + (timeNow.tv_usec);
	std::cout << "ReadingTime: "<< t2 -t1 << std::endl;
	
	return true;
}



