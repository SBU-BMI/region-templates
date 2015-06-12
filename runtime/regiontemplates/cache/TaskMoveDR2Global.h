

#ifndef TASKMOVEDR2GLOBAL_H_
#define TASKMOVEDR2GLOBAL_H_

#include "Task.h"
#include "Cache.h"


class TaskMoveDR2Global : public Task{
private:
	// this is a pointer to the Cache w/ this Worker
	Cache *curCache;
	std::string rtName, rtId, drName, drId;
	int timestamp, version;

	TaskMoveDR2Global();
public:
	TaskMoveDR2Global(Cache *curCache, std::string rtName, std::string rtId, std::string drName, std::string drId, int timestamp, int version);
	virtual ~TaskMoveDR2Global();

	//
	bool run(int procType=ExecEngineConstants::CPU, int tid=0);

};

#endif /* TASKMOVEDR2GLOBAL_H_ */
