/*
 * Cache.cpp
 *
 *  Created on: Aug 19, 2014
 *      Author: george
 */

#include "Cache.h"

Cache::Cache() {
	this->setWorkerId(-1);
}


Cache::~Cache() {
	for(int i = 0; i < this->cacheLayers.size(); i++){
		std::cout << "Cache: "<< i << "  hit: "<< this->hit[i] << " miss: "<< this->miss[i] <<std::endl;
	}
	std::cout << "Cache. Worker " <<this->getWorkerId() << " read time:" << this->readTime << " stage time: "<< this->stagingTime <<std::endl;
	std::cout << "Input misses: " << this->inputMiss << " read time: " << this->inputReadTime << std::endl;


	int i = this->cacheLayers.size();
	while(this->cacheLayers.size()>0){
		std::cout << "Deleting level: " << i << std::endl;
		i--;
		CacheComponent *comp = this->cacheLayers.back();
		delete comp;
		this->cacheLayers.pop_back();
	}
}

Cache::Cache(std::string confFile) {
	parseConfFile(confFile);

	this->setFirstGlobalCacheLevel(-1);
	// create and initialized one lock per cache layer
	for(int i = 0; i < this->cacheLayers.size(); i++){
		pthread_mutex_t auxLock;
		pthread_mutex_init(&auxLock, NULL);
		this->cacheLocks.push_back(auxLock);
		if(this->cacheLayers[i]->getType() == Cache::GLOBAL && this->getFirstGlobalCacheLevel() == -1){
			this->setFirstGlobalCacheLevel(i);
		}
		// instrumentation
		this->hit.push_back(0);
		this->miss.push_back(0);
	}
	// instrumentation
	this->readTime = 0;
	this->cacheOnRead = false;

	this->inputReadTime = 0;
	this->inputMiss = 0;
	pthread_mutex_init(&this->cacheInstLock, NULL);
	pthread_mutex_init(&this->globalLock, NULL);


	if(this->getFirstGlobalCacheLevel() == -1){
		std::cout << "WARNNING: no global cache LAYER" << std::endl;
	}
	this->setWorkerId(-1);
}

void Cache::parseConfFile(std::string confFile) {
	cv::FileStorage fs(confFile, cv::FileStorage::READ);
	if(fs.isOpened()){
		// mapping of level to cache component
		std::map<int, CacheComponent*> parserAux;

		std::cout << "Cache infra configuration:" << std::endl;
		cv::FileNode cacheConf = fs["CacheConfig"];

//		std::cout << "nodeSize: "<< cacheConf.size() << std::endl;
		cv::FileNodeIterator it = cacheConf.begin(), itEnd = cacheConf.end();
		int idx = 0;
		for(; it != itEnd; it++){
			std::cout << "type="<< (std::string)(*it)["type"] << " capacity="<< (int)(*it)["capacity"] << " path=" << (std::string)(*it)["path"] << " device="<< (std::string)(*it)["device"] << " level="<< (int)(*it)["level"]<< " policy="<< (std::string)(*it)["policy"]<< std::endl;
			std::string type_str = (std::string)(*it)["type"];
			int aux = (int)(*it)["capacity"];
			long capacity = aux;

			std::string device_str = (std::string)(*it)["device"];
			std::string path = (std::string)(*it)["path"];
			int level = (int)(*it)["level"];
			std::string replacementPolicy_str = (std::string)(*it)["policy"];

			int type;
			int device;
			int replacementPolicy;
			if(type_str.compare("LOCAL") == 0){
				type = Cache::LOCAL;
			}else{
				if(type_str.compare("GLOBAL") == 0){
					type = Cache::GLOBAL;
				}else{
					std::cout << "Unknown cache type: "<< type_str << ". It Should be either: LOCAL or GLOBAL."<< std::endl;
					exit(1);
				}
			}

			if(device_str.compare("RAM") == 0){
				device = Cache::RAM;
			}else{
				if(device_str.compare("SSD") == 0){
					device = Cache::SSD;
				}else{
					if(device_str.compare("HDD") == 0){
						device = Cache::HDD;
					}else{
						std::cout << "Unknown device : "<< device_str << ". It Should be either: RAM, SSD or HDD."<< std::endl;
						exit(1);
					}
				}
			}
			if(replacementPolicy_str.compare("FIFO") == 0){
				replacementPolicy = Cache::FIFO;
			}else{
				if(replacementPolicy_str.compare("LRU") == 0){
					replacementPolicy = Cache::LRU;
				}else{
					std::cout << "Unknown storage replacement policy : "<< replacementPolicy_str << ". It Should be either: FIFO or LRU."<< std::endl;
					exit(1);
				}
			}


			CacheComponent *cc = new CacheComponent(type, capacity, path, device, level, replacementPolicy);
			parserAux.insert(std::pair<int, CacheComponent*>(level, cc) );
		}

		for(int i = 1; i <= parserAux.size(); i++){
			std::map<int, CacheComponent*>::iterator it;
			it=parserAux.find(i);
			if(it != parserAux.end()){
				this->cacheLayers.push_back(it->second);
			}else{
				std::cout << "Cache layers must start from 1 and have increments of 1. Could not find cache layer: "<< i << std::endl;
				exit(1);
			}
		}
	}else{
		std::cout << "Could not find Cache config file (rtconf.xml)" << std::endl;
		exit(1);
	}
	return;
}


int Cache::insertDR(std::string rtName, std::string rtId, DataRegion* dataRegion, bool copyData, int startLayer, bool isCacheOnRead) {
//	std::cout << "Cache insertDR:" << dataRegion->getName() << std::endl;
	int insertedLevel = 0;
	long long init = Util::ClockGetTime();

	int i;
	// find first layer in which data fits
	for(i = startLayer; i < this->cacheLayers.size(); i++){
		if(dataRegion->getDataSize() < this->cacheLayers[i]->getCapacity()){
			break;
		}
	}

	// Data must fit in at least one layer
	assert(i != this->cacheLayers.size());

	this->cacheLayers[i]->lock();
	while(1){

		// TODO: include the case of a filesystem, which should not come here.
		// if should remove something from this cache layer. note that a single thread makes this test at time.
		if((this->cacheLayers[i]->getCapacity() - this->cacheLayers[i]->getCapacityUsed() < dataRegion->getDataSize()) && (this->cacheLayers[i]->cachePolicy->size() > 0)){
			// select data region for deletion
			DRKey drToDelete = this->cacheLayers[i]->cachePolicy->selectDelete();

			// delete data region that was moved to lower level
			this->cacheLayers[i]->deleteDataRegion(drToDelete.getRtName(), drToDelete.getRtId(), drToDelete.getDrName(), drToDelete.getDrId(), drToDelete.getTimeStamp(), drToDelete.getVersion());

		}else{
			// error check
			//assert(this->cacheLayers[i]->getCapacity() - this->cacheLayers[i]->getCapacityUsed() >= dataRegion->getDataSize());

			// set space used to include data region that will be inserted.
			this->cacheLayers[i]->setCapacityUsed(this->cacheLayers[i]->getCapacityUsed() + dataRegion->getDataSize());

			// there is space available
			break;
		}
	}
	// set information that is used to locate this data in the distributed cache.
	dataRegion->setCacheLevel(i);
	dataRegion->setCacheType(this->cacheLayers[i]->getType());
	dataRegion->setWorkerId(this->getWorkerId());
	insertedLevel = i;
//	std::cout << "Insert: level:"<<i<< " "<<dataRegion->getName()<< " "<< dataRegion->getId() << " size:"<<
//			dataRegion->getDataSize()<< " version: "<< dataRegion->getVersion() <<" empty? "<< dataRegion->empty()<< " capacity:"<<this->cacheLayers[i]->getCapacity()<<" used: "<<
//			this->cacheLayers[i]->getCapacityUsed()<<std::endl;
#ifdef DEBUG
	std::cout <<"FirstGlobal: "<< this->getFirstGlobalCacheLevel() << std::endl;
#endif
	if(this->cacheLayers[i]->getType() == Cache::GLOBAL){
		this->cacheLayers[i]->unlock();

		this->cacheLayers[i]->insertDR(rtName, rtId, dataRegion, copyData);

	}else{
		this->cacheLayers[i]->unlock();
		//i++;

		while(i < this->getFirstGlobalCacheLevel()){
#ifdef DEBUG
			std::cout << "InsertLevel: "<< i<< std::endl;
#endif
			this->cacheLayers[i]->insertDR(rtName, rtId, dataRegion, copyData);
			i++;
		}

		// write through: write to a global storage as well. If it's cache on read, then data is already on global
		if(!isCacheOnRead){
#ifdef DEBUG			
			std::cout << "InsertLevel: "<< i<< std::endl;
#endif
			this->cacheLayers[this->getFirstGlobalCacheLevel()]->insertDR(rtName, rtId, dataRegion, copyData);
		}
	}

	long long end = Util::ClockGetTime();
	pthread_mutex_lock(&this->cacheInstLock);
	this->stagingTime += end - init;
	pthread_mutex_unlock(&this->cacheInstLock);

	return insertedLevel;


/*	// find first layer in which data fits
	for(i = startLayer; i < this->cacheLayers.size(); i++){
		if(dataRegion->getDataSize() < this->cacheLayers[i]->getCapacity()){
			break;
		}
	}

	// Data must fit in at least one layer
	assert(i != this->cacheLayers.size());

	while(1){
		this->cacheLayers[i]->lock();

		// if should remove something from this cache layer. note that a single thread makes this test at time.
		if((this->cacheLayers[i]->getCapacity() - this->cacheLayers[i]->getCapacityUsed() < dataRegion->getDataSize()) && (this->cacheLayers[i]->cachePolicy->size() > 0)){
			// select data region for deletion
			DRKey drToDelete = this->cacheLayers[i]->cachePolicy->selectDelete();

			this->cacheLayers[i]->unlock();

			DataRegion* auxDelete = this->cacheLayers[i]->getDR(drToDelete.getRtName(), drToDelete.getRtId(), drToDelete.getDrName(), drToDelete.getDrId(), drToDelete.getTimeStamp(), drToDelete.getVersion(), true);

			this->cacheLayers[i]->lock();

			// delete data region that was moved to lower level
			this->cacheLayers[i]->deleteDataRegion(drToDelete.getRtName(), drToDelete.getRtId(), drToDelete.getDrName(), drToDelete.getDrId(), drToDelete.getTimeStamp(), drToDelete.getVersion());

			this->cacheLayers[i]->unlock();

			if(auxDelete != NULL){
				// if it is not app input we need to store data region into a lower level storage layer.
				if(auxDelete->getIsAppInput() == false){
					// Insert given data region into a lower level cache layer
					this->insertDR(drToDelete.getRtName(), drToDelete.getRtId(), auxDelete, true, i+1);

				}
				delete auxDelete;
			}
		}else{
			// error check
			assert(this->cacheLayers[i]->getCapacity() - this->cacheLayers[i]->getCapacityUsed() > dataRegion->getDataSize());

			// set space used to include data region that will be inserted.
			this->cacheLayers[i]->setCapacityUsed(this->cacheLayers[i]->getCapacityUsed() + dataRegion->getDataSize());

			this->cacheLayers[i]->unlock();
			// there is space available
			break;
		}

	}

	// set information that is used to locate this data in the distributed cache.
	dataRegion->setCacheLevel(i);
	dataRegion->setCacheType(this->cacheLayers[i]->getType());
	dataRegion->setWorkerId(this->getWorkerId());
	insertedLevel = i;
	std::cout << "Insert: level:"<<i<< " "<<dataRegion->getName()<< " "<< dataRegion->getId() << " size:"<<
			dataRegion->getDataSize()<< " version: "<< dataRegion->getVersion() <<" empty? "<< dataRegion->empty()<< " capacity:"<<this->cacheLayers[i]->getCapacity()<<" used: "<<
			this->cacheLayers[i]->getCapacityUsed()<<std::endl;
	this->cacheLayers[i]->insertDR(rtName, rtId, dataRegion, copyData);

	long long end = Util::ClockGetTime();
	pthread_mutex_lock(&this->cacheInstLock);
	this->stagingTime += end - init;
	pthread_mutex_unlock(&this->cacheInstLock);*/
	return insertedLevel;
}

// we need to know whether the data region is:
// 1) an application input data (isInput=true): in this case we will try to check whether this data is cached already, but
// if it is not found we'll access the global storage to read it. Currently we go to filesystem, but should change in the future
// to become more flexible
// 2) is local(isLocal=true) or global. If it is local, than it's metadata most be found in at least one of the cache component layers, if
// it is local and metadata is not found the DR was not inserted into the cache. It's empty.
// Otherwise, we will have to find it into a global storage.
DataRegion* Cache::getDR(std::string rtName, std::string rtId,
		std::string drName, std::string drId, int timestamp, int version, bool copyData, bool isInput, std::string inputPath) {
	DataRegion* retValue = NULL;
	long long init = Util::ClockGetTime();
	while(retValue == NULL){
		// loop through cache layers and try to find the data region.
		for(int i = 0; i < this->cacheLayers.size(); i++){
			// if DR is local to this cache or if cache layer is global, try to read it
			//			if(isLocal || this->cacheLayers[i]->getType() == Cache::GLOBAL){

			//this->cacheLayers[i]->lock();
			retValue = this->cacheLayers[i]->getDR(rtName, rtId, drName, drId, timestamp, version, copyData, isInput);
			//this->cacheLayers[i]->unlock();

			// update instrumentation
			if(retValue != NULL) {
				// time elapsed in read operation
				this->readTime += Util::ClockGetTime() - init;
				pthread_mutex_lock(&this->cacheInstLock);
				// it was missed in all caches in top levels
				for(int j = 0; j < i; j++){
					this->miss[j]++;
				}
				// found in level i
				this->hit[i]++;
				pthread_mutex_unlock(&this->cacheInstLock);

				break;
			}
			//pthread_mutex_unlock(&this->cacheLocks[i]);

			// if it is not on any cache layer and is an input, lets read it
			if(i+1 == this->cacheLayers.size() && isInput){
				if (drName == "POLY_LIST") {
					PolygonListDataRegion *polygonListDataRegion = new PolygonListDataRegion();
					polygonListDataRegion->setName(drName);
					polygonListDataRegion->setId(drId);
					polygonListDataRegion->setTimestamp(timestamp);
					polygonListDataRegion->setVersion(version);
					polygonListDataRegion->setIsAppInput(isInput);
					polygonListDataRegion->setInputFileName(inputPath);

					retValue = polygonListDataRegion;
				}
				else {
					DenseDataRegion2D *ddr2D = new DenseDataRegion2D();
					ddr2D->setName(drName);
					ddr2D->setId(drId);
					ddr2D->setTimestamp(timestamp);
					ddr2D->setVersion(version);
					ddr2D->setIsAppInput(isInput);
					ddr2D->setInputFileName(inputPath);

					retValue = ddr2D;
				}


				if(retValue->getType() == DataRegionType::DENSE_REGION_2D){
					DataRegionFactory::readDDR2DFS(dynamic_cast<DenseDataRegion2D*>(retValue), -1);
				//	std::cout << "READING INPUT DR. "<< retValue->getName() << " "<< retValue->getId() <<" "<< retValue->getTimestamp() << " "<< retValue->getVersion() << " inputFile: "<< inputPath<< std::endl;
				} else if (retValue->getType() == DataRegionType::POLYGON_LIST) {
					DataRegionFactory::readPolyListDRFS(dynamic_cast<PolygonListDataRegion *>(retValue));
				}
				else {
					std::cout << "Data region type: "<< retValue << " not supported"<< std::endl;
					exit(1);
				}
				if(retValue->empty()){
//					std::cout << "Empty input DR: " << retValue->getName() << " "<< retValue->getTimestamp() << " "<< retValue->getVersion() << std::endl;
					delete retValue;
					retValue = NULL;
				}else{
					pthread_mutex_lock(&this->cacheInstLock);
					// time elapsed in read operation
					this->inputReadTime += Util::ClockGetTime() - init;
					this->inputMiss++;
#ifdef DEBUG
					std::cout << "inputMiss:" << this->inputMiss << " read time: " << this->inputReadTime << std::endl;
#endif
					pthread_mutex_unlock(&this->cacheInstLock);

				}
				// cache data that is read into first level cache.
				if(cacheOnRead && this->cacheLayers[i]->getType() == Cache::GLOBAL && i != 0 && retValue != NULL){
					int firstCacheLevelToFit = 0;

					while( firstCacheLevelToFit < this->cacheLayers.size() && this->cacheLayers[firstCacheLevelToFit]->getCapacity() < retValue->getDataSize() ) firstCacheLevelToFit++;
					//std::cout << "FirstLevelToFit"
					if(firstCacheLevelToFit != i){
						// insert it into the first level layer
						this->insertDR(rtName, rtId, retValue, true, firstCacheLevelToFit, true);
						retValue->setWorkerId(this->getWorkerId());
					}else{
					//	std::cout << "firstLevelToFit is current. dr: "<< retValue->getName()<< std::endl;
					}

					// store space occupied by data. If it is a file based storage, data is not saved in memory and
					// this file is used to save such information. Therefore, the cache uses this field instead of
					// calculating the size from the data that may not be available in memory. Meaning, only metadata is on memory.
					retValue->setCachedDataSize(retValue->getDataSize());

				}

			}
			usleep(1000);
		}
	}
	if(retValue->empty()){
//		std::cout << "Empty DR: " << retValue->getName() << " "<< retValue->getTimestamp() << " "<< retValue->getVersion() << std::endl;
		delete retValue;
		retValue = NULL;
	}

	return retValue;
}

bool Cache::isCacheOnRead() const {
	return cacheOnRead;
}

void Cache::setCacheOnRead(bool cacheOnRead) {
	this->cacheOnRead = cacheOnRead;
}

int Cache::getCacheLevelType(int level) {
	int cacheType = -1;
	if(level < this->cacheLayers.size()){
		cacheType = this->cacheLayers[level]->getType();
	}
	return cacheType;
}

int Cache::getWorkerId() const {
	return workerId;
}

void Cache::setWorkerId(int workerId) {
	this->workerId = workerId;
}

bool Cache::move2Global(std::string rtName, std::string rtId, std::string drName,
		std::string drId, int timestamp, int version, int fromLevel, int toLevel) {
	bool retValue = false;
	int levelToStoreDR = toLevel;
	if(levelToStoreDR == -1){
		levelToStoreDR = this->getFirstGlobalCacheLevel();
	}
	assert(fromLevel >= 0 && fromLevel < this->cacheLayers.size());


	std::cout << "move2Global" << drName << ":"<< drId << ":"<< timestamp << ":"<< version << std::endl;
	DataRegion* auxDR2Move = NULL;
	// loop through cache layers and try to find the data region.
	for(int i = fromLevel; i < this->cacheLayers.size(); i++){

		// If it is already into a global cache component.
		if(this->cacheLayers[i]->getType() == Cache::GLOBAL){
			retValue = true;
			break;
		}

		// if data region is within this layer, get it from that cache layer
		auxDR2Move = this->cacheLayers[i]->getDR(rtName, rtId, drName, drId, timestamp, version, true, false);

		//auxDR2Move = this->cacheLayers[i]->getAndDelete(rtName, rtId, drName, drId, timestamp, version);

		if(auxDR2Move != NULL){
			std::cout << "move2Global: insert["<<i<<"]:" << rtName <<":" << rtId << std::endl;
			// try to insert into the first level with global context and so on.
			this->insertDR(rtName, rtId, auxDR2Move, true, this->getFirstGlobalCacheLevel());
			delete auxDR2Move;
			break;
		}
	}
	return retValue;
}

int Cache::getFirstGlobalCacheLevel() const {
	return firstGlobalCacheLevel;
}

void Cache::setFirstGlobalCacheLevel(int firstGlobalCacheLevel) {
	this->firstGlobalCacheLevel = firstGlobalCacheLevel;
}

bool Cache::deleteDR(std::string rtName, std::string rtId, std::string drName,
		std::string drId, int timestamp, int version) {
	bool retValue = false;
	for(int i = 0; i < this->cacheLayers.size(); i++){
		if(cacheLayers[i]->getType() == Cache::GLOBAL) break;

		pthread_mutex_lock(&this->cacheLocks[i]);

		// delete data region from cache
		retValue = this->cacheLayers[i]->deleteDataRegion(rtName, rtId, drName, drId, timestamp, version);

		pthread_mutex_unlock(&this->cacheLocks[i]);

		if(retValue) break;
	}
	return retValue;
}
