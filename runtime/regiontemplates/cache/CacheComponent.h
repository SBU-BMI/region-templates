/*
 * CacheComponent.h
 *
 *  Created on: Aug 19, 2014
 *      Author: george
 */

#ifndef CACHECOMPONENT_H_
#define CACHECOMPONENT_H_

#include "DataRegion.h"
#include "CachePolicy.h"
#include "CachePolicyLRU.h"
#include "Cache.h"
#include "DenseDataRegion2D.h"
#include "DataRegionFactory.h"
#include <list>

class CacheComponent {
private:
	int type;
	long capacity;
	std::string path;
	int device;
	int level;
	bool deleteFile;

	// variables used the control data memory usage.
	long capacityUsed;

	virtual bool deleteDataRegion(std::string rtName, std::string rtId, std::string drName,  std::string drId, int timestamp=0, int version=0);

	pthread_mutex_t cacheComponentsLock;
	friend class Cache;

protected:
	CacheComponent();

	// determine which is the next region template to be deleted according
	// to the operations performed in the cache: insert and get.
	CachePolicy* cachePolicy;

	std::map<std::string, std::list<DataRegion*> > drCache;

	void setCapacity(long capacity);
	long getCapacityUsed() const;
	void setCapacityUsed(long capacityUsed);

	void lock();
	void unlock();

public:
	CacheComponent(int type, long capacity, std::string path, int device, int level, int replacementPolicy, bool deleteFile=false);
	virtual ~CacheComponent();

	virtual bool insertDR(std::string rtName, std::string rtId, DataRegion* dataRegion, bool copyData=true);

	virtual DataRegion *getDR(std::string rtName, std::string rtId, std::string drName, std::string drId,
							  std::string inputFileName, int timestamp = 0, int version = 0, bool copyData = true,
							  bool isLocal = true);

	// remove data region from cache and return it. GetDR followed by deleteDR. returnd the DR or NULL if its not found
	virtual DataRegion *getAndDelete(std::string rtName, std::string rtId, std::string drName,
									 std::string inputFileName, std::string drId, int timestamp = 0, int version = 0);

	long getCapacity() const;

	int getDevice() const;
	void setDevice(int device);
	std::string getPath() const;
	void setPath(std::string path);
	int getType() const;
	void setType(int type);
	int getLevel() const;
	void setLevel(int level);

};

#endif /* CACHECOMPONENT_H_ */
