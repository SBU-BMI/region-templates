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
		int device, int level, bool deleteFile): type(type), path(path), device(device), level(level), deleteFile(deleteFile) {
	this->setCapacity(capacity*1024*1024);
	this->setCapacityUsed(0);
	this->cachePolicy = new CachePolicy();
	pthread_mutex_init(&this->cacheComponentsLock, NULL);
}

CacheComponent::~CacheComponent() {

	// clean data cache: delete every single data region stored
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

	// found rt. Now check if a given data regio with same version/name/timestamp exists
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
				std::cout << "deleteDataregion: "<< (*listIt)->getName() << ":"<< (*listIt)->getId() <<":"<< (*listIt)->getDataSize() << std::endl;
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
	// create key of the cache: RT name + RT id
	std::string rtGlobalId = rtName;
	rtGlobalId.append(rtId);

	// clone data region
	DataRegion *drClone;
	if(this->getDevice() == Cache::RAM){
		drClone = dataRegion->clone(copyData);
	}else{
		// for disk based storage that data will be stored in disk. No need to create a data copy.
		drClone = dataRegion->clone(false);

	}
	// store space occupied by data. If it is a file based storage, data is not saved in memory and
	// this filed is used to save such information. Therefore, the cache uses this field instead of
	// calculating the size from the data that may not be available in memory
	drClone->setCachedDataSize(dataRegion->getDataSize());

	// this single data region is larger than the cache capacity
	if(dataRegion->getDataSize() > this->getCapacity()){
		delete drClone;
		return false;
	}

	// lock cache for modification
//	pthread_mutex_lock(&this->cacheComponentsLock);

	// if the given data region (same id<drName, timestamp, version>) is already in cache, delete it.
	this->deleteDataRegion(rtName, rtId, dataRegion->getName(), dataRegion->getId(), dataRegion->getTimestamp(), dataRegion->getVersion());

	// check if there is available space in cache, and release other data regions if necessary
	if(this->getCapacity() - this->getCapacityUsed() < dataRegion->getDataSize()){
		// there is no space available. release data.
		std::cout << "TODO: release data when cache is FULL" << std::endl;
	}
	// set space used to include data region that will be inserted.
	this->setCapacityUsed(this->getCapacityUsed() + dataRegion->getDataSize());

	// inform cache policy of the data insertion
	this->cachePolicy->insertItem(DRKey(rtName, rtId, dataRegion->getName(), dataRegion->getId(), dataRegion->getTimestamp(),dataRegion->getVersion()));

	// Now, there is space available and another DR w/ same id is not in cache. We can insert current DR.
	// try to find the given RT
	cacheIt = this->drCache.find(rtGlobalId);

	// if FileSystem based caching, write the data to file and keep only the metadata cached.
	if(this->getDevice() == Cache::SSD || this->getDevice() == Cache::HDD){

		if(DataRegionType::DENSE_REGION_2D == dataRegion->getType()){
			DenseDataRegion2D *aux = dynamic_cast<DenseDataRegion2D*>(dataRegion);
			DataRegionFactory::writeDDR2DFS(aux,this->getPath());
		}else{
			std::cout << "ERROR: Unknown data region type: "<< dataRegion->getType() << std::endl;
			exit(1);
		}
	}

	std::cout << "Inserting DR: " << dataRegion->getName() << " id: "<< dataRegion->getId() <<" "<< dataRegion->getTimestamp() << " "<<dataRegion->getVersion()<< std::endl;
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

	// unlock cache access
//	pthread_mutex_unlock(&this->cacheComponentsLock);
	return true;
}

DataRegion* CacheComponent::getDR(std::string rtName, std::string rtId,
		std::string drName, std::string drId, int timestamp, int version, bool copyData, bool isAppInput) {
	// cache iteration definition
	std::map<std::string, std::list<DataRegion*> >::iterator cacheIt;
	DataRegion* retValue = NULL;
	// create key of the cache: RT name + RT id
	std::string rtGlobalId = rtName;
	rtGlobalId.append(rtId);
	// lock cache for modification
	//	pthread_mutex_lock(&this->cacheComponentsLock);
//	if(isLocal){

		// try to find the given RT
		cacheIt = this->drCache.find(rtGlobalId);

		// found rt. Now check if a given data region with same version/name/timestamp exists
		if(cacheIt != this->drCache.end()){
			// try to find a region with same version and timestamp.
			std::list<DataRegion*>::iterator listIt = (*cacheIt).second.begin();

			// check if a region with same version and timestamp exist
			for(; listIt != (*cacheIt).second.end(); listIt++){

				// if found the specific region
				if(		drName.compare((*listIt)->getName()) == 0 	&&
						drId.compare((*listIt)->getId()) == 0 		&&
						timestamp == (*listIt)->getTimestamp() 		&&
						version == (*listIt)->getVersion() ){

					// inform cache policy of the data access
					this->cachePolicy->accessItem(DRKey(rtName, rtId, (*listIt)->getName(), (*listIt)->getId(), (*listIt)->getTimestamp(),(*listIt)->getVersion()));

					//// if FileSystem based caching, write the data to file and keep only the metadata cached.
					//if(this->getDevice() == Cache::SSD || this->getDevice() == Cache::HDD){
					switch(this->getDevice()){

					case Cache::SSD:
					case Cache::HDD:
						retValue = (*listIt)->clone(false);
						if(retValue->getType() == DataRegionType::DENSE_REGION_2D){
							DataRegionFactory::readDDR2DFS(dynamic_cast<DenseDataRegion2D*>(retValue), -1, this->getPath());
						}else{
							std::cout << "Data region type: "<< retValue << " not supported"<< std::endl;
							exit(1);
						}

						break;
					case Cache::RAM:
						// clone data region to return it
						retValue = (*listIt)->clone(copyData);
						break;
					default:
						std::cout << __FILE__ << ":"<< __LINE__<< ". ERROR: Unknown device type: "<< this->getDevice() << std::endl;

					}
				}
			}
		}
		//else{
			// non local data region. it would not be found in the cache metadata, but should be read
			if(retValue == NULL && this->getType() == Cache::GLOBAL){
				DenseDataRegion2D * ddr2D= new DenseDataRegion2D();
				ddr2D->setName(drName);
				ddr2D->setId(drId);
				ddr2D->setTimestamp(timestamp);
				ddr2D->setVersion(version);
				//ddr2D->setIsAppInput(isAppInput);

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
			}
		//}


	// unlock cache access
	//	pthread_mutex_unlock(&this->cacheComponentsLock);

	return retValue;
}

DataRegion* CacheComponent::getAndDelete(std::string rtName, std::string rtId,
		std::string drName, std::string drId, int timestamp, int version) {
	DataRegion* dr = this->getDR(rtName, rtId, drName, drId, timestamp, version, true);
	if(dr!=NULL){
		this->deleteDataRegion(rtName, rtId, drName, drId, timestamp, version);
	}
	return dr;
}
