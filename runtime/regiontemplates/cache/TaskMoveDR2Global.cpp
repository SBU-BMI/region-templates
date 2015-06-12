

#include "TaskMoveDR2Global.h"

TaskMoveDR2Global::TaskMoveDR2Global() {

}

TaskMoveDR2Global::~TaskMoveDR2Global() {

}

TaskMoveDR2Global::TaskMoveDR2Global(Cache *curCache, std::string rtName, std::string rtId, std::string drName, std::string drId, int timestamp, int version) {
	this->curCache = curCache;
	this->rtName = rtName;
	this->rtId = rtId;
	this->drName = drName,
	this->drId = drId;
	this->timestamp = timestamp;
	this->version = version;
	this->setTaskType(ExecEngineConstants::RT_CACHE_DUMP);
}

bool TaskMoveDR2Global::run(int procType, int tid) {
	std::cout << "Move DR to Global: " << drName <<":"<<drId<<":"<<timestamp<<":"<<version<<std::endl;

	//pthread_mutex_lock(&this->curCache->globalLock);
	this->curCache->move2Global(rtName, rtId, drName, drId, timestamp, version);
	
	//pthread_mutex_unlock(&this->curCache->globalLock);
	return true;
}
