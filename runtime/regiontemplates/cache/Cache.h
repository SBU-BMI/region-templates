/*
 * Cache.h
 *
 *  Created on: Aug 19, 2014
 *      Author: george
 */

#ifndef CACHE_H_
#define CACHE_H_

#include "opencv2/opencv.hpp"
#include "CacheComponent.h"
#include <unistd.h>
#include "Util.h"
//#include "Worker.h"

class CacheComponent;
// This is the cache infrastructure
class Cache {
private:
	Cache();
	std::vector<CacheComponent*> cacheLayers;
	// Per layer cache lock.
	std::vector<pthread_mutex_t> cacheLocks;
	std::vector<int> hit;
	std::vector<int> miss;
	long long readTime;

	// cache instrumentation lock
	pthread_mutex_t cacheInstLock;
	int inputMiss;
	long long inputReadTime;
	long long stagingTime;

	// cache data read from global?
	bool cacheOnRead;

	void parseConfFile(std::string confFile);

	int firstGlobalCacheLevel;
	int getFirstGlobalCacheLevel() const;
	void setFirstGlobalCacheLevel(int firstGlobalCacheLevel);

	friend class Worker;
	friend class RegionTemplate;
protected:
	// Id of the which this cache is associated
	int workerId;

	int getWorkerId() const;
	void setWorkerId(int workerId);

	bool isCacheOnRead() const;
	void setCacheOnRead(bool cacheOnRead);

public:

	pthread_mutex_t globalLock;
	Cache(std::string confFile);
	virtual ~Cache();

	// return the level in which it was stored.
	int insertDR(std::string rtName, std::string rtId, DataRegion* dataRegion, bool copyData=true,  int startLayer=0, bool isCacheOnRead=false);
	DataRegion *getDR(std::string rtName, std::string rtId, std::string drName, std::string drId, std::string inputName,
					  int timestamp = 0, int version = 0, int drType = DataRegionType::DENSE_REGION_2D, bool copyData = true, bool isLocal = true,
					  std::string inputPath = "");
	bool deleteDR(std::string rtName, std::string rtId, std::string drName, std::string drId, int timestamp=0, int version=0);
	int getCacheLevelType(int level); // is it global/local.

	// move data region to a cache component of global scope. Level if a specific storage layer should be used
	bool move2Global(std::string rtName, std::string rtId, std::string drName, std::string drId,
					 std::string inputFileName, int timestamp = 0, int version = 0, int fromLevel = 0,
					 int toLevel = -1);


	// type
	static const int GLOBAL = 1; // accessible from other nodes
	static const int LOCAL = 2; // accessible within the node that stored the data

	// device
	static const int RAM = 1;
	static const int SSD = 2;
	static const int HDD = 3;

	// replacement policy
	static const int FIFO = 1;
	static const int LRU  = 2;


};

#endif /* CACHE_H_ */
