/*
 * CacheComponent.cpp
 *
 *  Created on: Aug 19, 2014
 *      Author: george
 */

#include "CacheComponent.h"
// this is private and, as such, never used.
CacheComponent::CacheComponent() {
	this->cachePolicy = new CachePolicy();
}


CacheComponent::CacheComponent(int type, long capacity, std::string path,
		int device, int level,  int replacementPolicy, bool deleteFile): type(type), path(path), device(device), level(level),  deleteFile(deleteFile) {
	this->setCapacity(capacity*1024*1024);
	this->setCapacityUsed(0);
	switch(replacementPolicy){
	case Cache::FIFO:
		this->cachePolicy = new CachePolicy();
		break;
	case Cache::LRU:
		this->cachePolicy = new CachePolicyLRU();
		break;
	default:
		std::cout << "Unknown cache replacement policy: "<< replacementPolicy << std::endl;
		exit(1);
	}

	pthread_mutex_init(&this->cacheComponentsLock, NULL);
}

CacheComponent::~CacheComponent() {

/*	// clean data cache: delete every single data region stored
	while(this->cachePolicy->size() > 0){
		// select data region for deletion
		DRKey drToDelete = this->cachePolicy->selectDelete();
#ifdef DEBUG
		std::cout << "~CacheComponent:SELECTED DR for deletion:" << std::endl;
		drToDelete.print();
#endif
		this->deleteDataRegion(drToDelete.getRtName(), drToDelete.getRtId(), drToDelete.getDrName(), drToDelete.getDrId(), drToDelete.getTimeStamp(), drToDelete.getVersion());
	}

#ifdef DEBUG
	// error check
	if(this->getCapacityUsed() != 0){
		std::cout << "~CacheComponent: ERROR: deleted every single data region using cache policy, but still its not empty:"<< this->getCapacityUsed() << std::endl;
	}else{
		std::cout << "Cache is empty." << std::endl;
	}
#endif

*/
	pthread_mutex_destroy(&this->cacheComponentsLock);
	delete this->cachePolicy;
}

long CacheComponent::getCapacity() const {
	return capacity;
}

void CacheComponent::setCapacity(long capacity) {
	this->capacity = capacity;
}

int CacheComponent::getDevice() const {
	return device;
}

void CacheComponent::setDevice(int device) {
	this->device = device;
}

std::string CacheComponent::getPath() const {
	return path;
}

void CacheComponent::setPath(std::string path) {
	this->path = path;
}

int CacheComponent::getType() const {
	return type;
}

void CacheComponent::setType(int type) {
	this->type = type;
}

int CacheComponent::getLevel() const {
	return level;
}

long CacheComponent::getCapacityUsed() const {
	return capacityUsed;
}

void CacheComponent::setCapacityUsed(long capacityUsed) {
	this->capacityUsed = capacityUsed;
}

bool CacheComponent::deleteDataRegion(std::string rtName, std::string rtId,
		std::string drName, std::string drId, int timestamp, int version) {
	bool retValue = false;

	// create global RT name ID
	std::string rtGlobalId = rtName;
	rtGlobalId.append(rtId);

	std::map<std::string, std::list<DataRegion*> >::iterator cacheIt;


	// try to find the given RT
	cacheIt = this->drCache.find(rtGlobalId);

	// found rt. Now check if a given data region with same version/name/timestamp exists
	if(cacheIt != this->drCache.end()){
		// try to find a region with same version and timestamp.
		std::list<DataRegion*>::iterator listIt = (*cacheIt).second.begin();

		// check if a region with same version and timestamp exist
		for(; listIt != (*cacheIt).second.end(); listIt++){

			// if found the specific region
			if(		drName.compare((*listIt)->getName()) == 0 &&
					drId.compare((*listIt)->getId()) == 0 &&
					timestamp == (*listIt)->getTimestamp() &&
					version == (*listIt)->getVersion() ){
				// delete data region
				// First, retrieve space used to cache data region
				this->setCapacityUsed(this->getCapacityUsed() - (*listIt)->getCachedDataSize());

				// this is going to happen for file system based cache.
/*				if(deleteFile && (this->getDevice() == Cache::SSD || this->getDevice() == Cache::HDD)){
					// file name w/ path
					std::string fullFileName = this->getPath();
					fullFileName.append((*listIt)->getId());
					if(remove(fullFileName.c_str()) != 0 ){
						std::cout << "Error deleting file: "<< fullFileName << std::endl;
					}
				}*/
#ifdef DEBUG
				std::cout << "deleteDataregion: "<< drName << " version:"<< version <<" timestamp:"<< timestamp << std::endl;
				std::cout << "deleteDataregion: "<< (*listIt)->getName() << " version:"<< (*listIt)->getVersion() <<" timestamp:"<< (*listIt)->getTimestamp() << std::endl;
#endif
				// delete data region object
				delete (*listIt);
				// remove data region entry from list
				(*cacheIt).second.erase(listIt);

				// if there is no data region stored w/ this region
				// template key, lets remove it from the cache map
				if((*cacheIt).second.size() == 0){
					this->drCache.erase(cacheIt);
				}
				retValue = true;
				break;
			}
		}
	}



	return retValue;
}

void CacheComponent::setLevel(int level) {
	this->level = level;
}

bool CacheComponent::insertDR(std::string rtName, std::string rtId, DataRegion* dataRegion, bool copyData) {
	std::map<std::string, std::list<DataRegion*> >::iterator cacheIt;

	if(this->getDevice() == Cache::RAM){
		// create key of the cache: RT name + RT id
		std::string rtGlobalId = rtName;
		rtGlobalId.append(rtId);

		assert(dataRegion!=NULL);
		// clone data region
		DataRegion *drClone;
		if(this->getDevice() == Cache::RAM){
			drClone = dataRegion->clone(copyData);
		}else{
			// for disk based storage that data will be stored in disk. No need to create a data copy.
			drClone = dataRegion->clone(false);
		}
		// store space occupied by data. If it is a file based storage, data is not saved in memory and
		// file is used to save such information. Therefore, the cache uses this field instead of
		// calculating the size from the data that may not be available in memory
		drClone->setCachedDataSize(dataRegion->getDataSize());

		// lock for possible modification in cache component state
		this->lock();

//		std::cout << "InsertDr: call delete. drName: "<< dataRegion->getName() << " timestamp: "<< dataRegion->getTimestamp() <<" version:"<< dataRegion->getVersion()<< std::endl;
		// if a data region (same id<drName, timestamp, version>) is already in cache, delete it.
		this->deleteDataRegion(rtName, rtId, dataRegion->getName(), dataRegion->getId(), dataRegion->getTimestamp(), dataRegion->getVersion());

		// inform cache policy of the data insertion
		this->cachePolicy->insertItem(DRKey(rtName, rtId, dataRegion->getName(), dataRegion->getId(), dataRegion->getTimestamp(),dataRegion->getVersion()));

		// Now, there is space available and another DR w/ same id is not in cache. We can insert current DR.
		// try to find the given RT
		cacheIt = this->drCache.find(rtGlobalId);
		// found rt. Now insert the data regions into its list
		if(cacheIt != this->drCache.end()){
			// insert received data region into the cache
			(*cacheIt).second.push_front(drClone);
		}else{
			// did not find key. No RT w/ this name exists. Let's create it.
			std::list<DataRegion*> drList;
			drList.push_back(drClone);
			this->drCache.insert(std::pair<std::string, std::list<DataRegion*> >(rtGlobalId, drList));
		}
		this->unlock();
	}


	// if FileSystem based caching, write the data to file and keep only the metadata cached.
	if(this->getDevice() == Cache::SSD || this->getDevice() == Cache::HDD){
//		std::cout << "InsertComponent: "<< dataRegion->getName() <<" version: "<<dataRegion->getVersion()<< " rows:"<< (dynamic_cast<DenseDataRegion2D*>(dataRegion))->getData().rows << std::endl;
//		if(DataRegionType::DENSE_REGION_2D == dataRegion->getType()){
	//		DenseDataRegion2D *aux = dynamic_cast<DenseDataRegion2D*>(dataRegion);
			DataRegionFactory::writeDDR2DFS(dataRegion,this->getPath(),this->getDevice() == Cache::SSD);
//			 std::cout<< "Cache write comp path: "<< this->getPath()<< std::endl;
//		}else{
//			std::cout << "ERROR: Unknown data region type: "<< dataRegion->getType() << std::endl;
//			exit(1);
//		}
	}

	std::cout << "[CacheComponent] Inserting DR: " << dataRegion->getName() << " id: "<< dataRegion->getId() <<" "<< dataRegion->getTimestamp() << " "<<dataRegion->getVersion()<< std::endl;


	return true;
}

DataRegion* CacheComponent::getDR(std::string rtName, std::string rtId,
								  std::string drName, std::string drId, std::string inputFileName, int timestamp,
								  int version, bool copyData, bool isAppInput) {
	// cache iteration definition
	std::map<std::string, std::list<DataRegion*> >::iterator cacheIt;
	DataRegion* retValue = NULL;
	// create key of the cache: RT name + RT id
	std::string rtGlobalId = rtName;
	rtGlobalId.append(rtId);

	// lock cache for modification
	//	this->lock();
	switch(this->getDevice()){

	case Cache::SSD:
	case Cache::HDD:
	{
		// Create generic data region to pass arguments.
		retValue = new DataRegion();
		retValue->setName(drName);
		retValue->setId(drId);
		retValue->setTimestamp(timestamp);
		retValue->setVersion(version);
		retValue->setInputFileName(inputFileName);

		bool retDRF = DataRegionFactory::readDDR2DFS(retValue, &retValue, -1, this->getPath(), this->getDevice() == Cache::SSD);

		if(retDRF == false) {
			delete retValue;
			retValue = NULL;
		}

		break;
	}
	case Cache::RAM:
	{
		this->lock();
		// try to find the given RT
		cacheIt = this->drCache.find(rtGlobalId);

		// found rt. Now check if a given data region with same version/name/timestamp exists
		if(cacheIt != this->drCache.end()){
			// try to find a region with same version and timestamp.
			std::list<DataRegion*>::iterator listIt = (*cacheIt).second.begin();
			std::list<DataRegion*>::iterator listItEnd = (*cacheIt).second.end();

			// check if a region with same version and timestamp exist
			for(; listIt != (*cacheIt).second.end(); listIt++){

				// if found the specific region
				if(		drName.compare((*listIt)->getName()) == 0 	&&
						drId.compare((*listIt)->getId()) == 0 		&&
						timestamp == (*listIt)->getTimestamp() 		&&
						version == (*listIt)->getVersion() ){

					// inform cache policy of the data access
					this->cachePolicy->accessItem(DRKey(rtName, rtId, (*listIt)->getName(), (*listIt)->getId(), (*listIt)->getTimestamp(),(*listIt)->getVersion()));
					// clone data region to return it
					retValue = (*listIt)->clone(copyData);
				}
			}
		}
		this->unlock();
		break;
	}
	default:
	{
		std::cout << __FILE__ << ":"<< __LINE__<< ". ERROR: Unknown device type: "<< this->getDevice() << std::endl;
		//this->unlock();

		break;
	}
	}



//	// try to find the given RT
//	cacheIt = this->drCache.find(rtGlobalId);
//
//	// found rt. Now check if a given data region with same version/name/timestamp exists
//	if(cacheIt != this->drCache.end()){
//		// try to find a region with same version and timestamp.
//		std::list<DataRegion*>::iterator listIt = (*cacheIt).second.begin();
//		std::list<DataRegion*>::iterator listItEnd = (*cacheIt).second.end();
//
//		// check if a region with same version and timestamp exist
//		for(; listIt != (*cacheIt).second.end(); listIt++){
//
//			// if found the specific region
//			if(		drName.compare((*listIt)->getName()) == 0 	&&
//					drId.compare((*listIt)->getId()) == 0 		&&
//					timestamp == (*listIt)->getTimestamp() 		&&
//					version == (*listIt)->getVersion() ){
//
//				// inform cache policy of the data access
//				this->cachePolicy->accessItem(DRKey(rtName, rtId, (*listIt)->getName(), (*listIt)->getId(), (*listIt)->getTimestamp(),(*listIt)->getVersion()));
//
//				//// if FileSystem based caching, write the data to file and keep only the metadata cached.
//				//if(this->getDevice() == Cache::SSD || this->getDevice() == Cache::HDD){
//				switch(this->getDevice()){
//
//				case Cache::SSD:
//				case Cache::HDD:
//					retValue = (*listIt)->clone(false);
//					//this->unlock();
//
//					// HD reading may take place in parallel w/ other operations
//					if(retValue->getType() == DataRegionType::DENSE_REGION_2D){
//						DataRegionFactory::readDDR2DFS(dynamic_cast<DenseDataRegion2D*>(retValue), -1, this->getPath());
//					}else{
//						std::cout << "Data region type: "<< retValue << " not supported"<< std::endl;
//						exit(1);
//					}
//
//					break;
//				case Cache::RAM:
//
//					// clone data region to return it
//					retValue = (*listIt)->clone(copyData);
//
//					//this->unlock();
//
//					break;
//				default:
//					std::cout << __FILE__ << ":"<< __LINE__<< ". ERROR: Unknown device type: "<< this->getDevice() << std::endl;
//					//this->unlock();
//
//				}
//			}
//		}
//	}else{
//
//	}
//		// did not find data region in previous loop
//		//if(listIt == listItEnd) this->unlock();
//	}else{
//		//this->unlock();
//	}

	// no need for lock here
	// non local data region. it would not be found in the cache metadata, but should be read
/*	if(retValue == NULL && this->getType() == Cache::GLOBAL){
		DenseDataRegion2D * ddr2D= new DenseDataRegion2D();
		ddr2D->setName(drName);
		ddr2D->setId(drId);
		ddr2D->setTimestamp(timestamp);
		ddr2D->setVersion(version);

		switch(this->getDevice()){
		case Cache::SSD:
		case Cache::HDD:
			retValue = ddr2D;
			if(retValue->getType() == DataRegionType::DENSE_REGION_2D){
				bool retDRF = DataRegionFactory::readDDR2DFS(dynamic_cast<DenseDataRegion2D*>(retValue), -1, this->getPath());

				std::cout << "READING GLOBAL DR. "<< retValue->getName() << " "<< retValue->getId() <<" "<< retValue->getTimestamp() << " "<< retValue->getVersion() << std::endl;
				// did not find .lock, means data was not written to global storage
				if(retDRF == false){
					delete retValue;
					retValue = NULL;
				}
			}else{
				std::cout << "Data region type: "<< retValue << " not supported"<< std::endl;
				exit(1);
			}
			break;
		default:
			std::cout << __FILE__ << ":"<< __LINE__<< ". ERROR: Unknown device type: "<< this->getDevice() << std::endl;

		}
	}*/

	return retValue;
}

DataRegion* CacheComponent::getAndDelete(std::string rtName, std::string rtId,
										 std::string drName, std::string drId, std::string inputFileName, int timestamp,
										 int version) {
	DataRegion *dr = this->getDR(rtName, rtId, drName, drId, inputFileName, timestamp, version, true);
	if(dr!=NULL){
		//this->lock();
		this->deleteDataRegion(rtName, rtId, drName, drId, timestamp, version);
		//this->unlock();
	}
	return dr;
}

void CacheComponent::lock() {
	pthread_mutex_lock(&this->cacheComponentsLock);
}

void CacheComponent::unlock() {
	pthread_mutex_unlock(&this->cacheComponentsLock);
}
