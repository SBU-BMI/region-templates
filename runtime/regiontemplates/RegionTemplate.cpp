/*
 * RegionTemplate.cpp
 *
 *  Created on: Oct 22, 2012
 *      Author: george
 */

#include "RegionTemplate.h"

void RegionTemplate::printRT() {
	for (std::map<std::string, std::list<DataRegion*> >::iterator r = templateRegions.begin(); r!=templateRegions.end(); r++) {
		cout << "RT: " << r->first << endl;
		for (std::list<DataRegion*>::iterator d = r->second.begin(); d!=r->second.end(); d++) {
			cout << "\tDR: " << (*d)->getName() << endl;
		}
	}
}


RegionTemplate::RegionTemplate() {
//	lazyRead = false;
	this->setLazyRead(false);
	this->setLocation(PipelineComponentBase::MANAGER_SIDE);
}

RegionTemplate::~RegionTemplate() {

	// Clean up regions stored within this region templates
	std::map<std::string, std::list<DataRegion*> >::iterator it = this->templateRegions.begin();

	// For each region
	while(it != this->templateRegions.end()){

		// Iterate in the list of data regions with same name
		std::list<DataRegion*>::iterator listIt = (*it).second.begin();

	//	std::cout << "~RegionTemplate: " << this->getName() << " nDataRegions: " << this->getNumDataRegions() << std::endl;

		while( listIt != (*it).second.end() ){		
		//	std::cout << "deleting region:"<< (*listIt)->getName() <<std::endl;
			// delete each item in the current list
			DataRegion *dr = (*listIt);
			delete (dr);
			listIt++;
		}
		it++;
	}
}

const BoundingBox& RegionTemplate::getBb() const {
	return bb;
}

void RegionTemplate::setBb(const BoundingBox& bb) {
	this->bb = bb;
}

std::string RegionTemplate::getName() const {
	return name;
}

void RegionTemplate::setName(std::string name) {
	this->name = name;
}

unsigned int RegionTemplate::getVersion() const {
	return version;
}

int RegionTemplate::getTimestamp() const {
	return timestamp;
}

bool RegionTemplate::insertDataRegion(DataRegion* dataRegion) {
	bool retValue = true;
#ifdef DEBUG
	std::cout << "DR: "<< dataRegion->getName() << " time: "<< dataRegion->getTimestamp()
			<< " version: " << dataRegion->getVersion() << std::endl;
#endif
	// it starts w/ false when component is created, and is changed to true when user inserts a data region.
	dataRegion->setModified(true);
	// The name can't be empty
	if(dataRegion->getName().size() > 0){
		std::map<std::string, std::list<DataRegion*> >::iterator it;
		it = templateRegions.find(dataRegion->getName());

		// there are already regions in the map with this name
		if(it != templateRegions.end()){
			// Iterator for the list of data regions with same name
			std::list<DataRegion*>::iterator listIt = (*it).second.begin();

			// check if a region with same version and timestamp exists
			for(; listIt != (*it).second.end(); listIt++){
				if(dataRegion->getTimestamp() == (*listIt)->getTimestamp() &&
						dataRegion->getVersion() == (*listIt)->getVersion() ){
					std::cout << "Warning: DataRegion with same version and timestamp is stored. It will be deleted" << std::endl;
					delete (*listIt);
					(*it).second.erase(listIt);
					break;
				}
			}

			// I expect this data region to be accessed soon after inserted here.
			// Putting it into the head of the list
			(*it).second.push_front(dataRegion);
		}else{
			std::list<DataRegion*> dataRegionList;
			dataRegionList.push_back(dataRegion);
			templateRegions.insert(std::pair<std::string,std::list<DataRegion*> >(dataRegion->getName(), dataRegionList));
		}

	}else{
		// If the name(ID) of the region is empty there is nothing we can do.
		std::cout << "Warning: tried to insert a data region with empty name in a region template. It was not inserted!" << std::endl;
		retValue = false;
	}
	return retValue;
}

// Return a specific region within this template that is specified by the triple
DataRegion* RegionTemplate::getDataRegion(std::string drName, std::string drId, int timestamp, int version) {
	// Did not find the region requested by default
	DataRegion *retValue = NULL;

	// friendly iterator to walk through the list of regions with same name (equals to parameter 'name')
	std::map<std::string, std::list<DataRegion*> >::iterator it;

	// search for required region within this region template
	it = templateRegions.find(drName);

	// if we have found regions with that  name
	if(it != templateRegions.end()){
		// try to find a region with same version and timestamp. This should be a happy matching
		std::list<DataRegion*>::iterator listIt = (*it).second.begin();

		// check if a region with same version and timestamp exist
		for(; listIt != (*it).second.end(); listIt++){
//			std::cout << " Number of data regions: "<< (*it).second.size() << std::endl;
			// if found the specific region. If drId is empty, any id is accepted
			if( (drId.compare((*listIt)->getId()) == 0 || drId.empty())&&
					timestamp == (*listIt)->getTimestamp() &&
					version == (*listIt)->getVersion() ){
				// set the return value as to that region
				retValue = (*listIt);
//				std::cout << "found: "<< retValue->getName() << " "<< retValue->getId() <<" time: "<< retValue->getTimestamp()<< " version: "<< retValue->getVersion()<< std::endl;
				// if required data region has not been loaded
				if(retValue->empty() && (this->getLocation() == PipelineComponentBase::WORKER_SIDE)){
					long long endRead, initRead = Util::ClockGetTime();

					Cache* auxCache = this->getCache();
					//pthread_mutex_lock(&auxCache->globalLock);
					if(auxCache != NULL){
#ifdef DEBUG
						std::cout << "auxCache->getDR: " << this->getName() << ":"<< this->getId() << ":"<< retValue->getName() << ":"<<  retValue->getId() << ":"<<  retValue->getTimestamp() << ":"<<  retValue->getVersion() << ":"<<  true << ":"<<  auxCache->getWorkerId() << ":"<<  retValue->getWorkerId() << std::endl;
#endif

						retValue = auxCache->getDR(this->getName(), this->getId(), retValue->getName(), retValue->getId(), retValue->getTimestamp(), retValue->getVersion(), true, retValue->getIsAppInput(), retValue->getInputFileName());

					}else{
						std::cout << "RT::getDataRegion: CACHE is NULL. rtName: "<< this->getName() << std::endl;
						//exit(1);
					}

					//pthread_mutex_unlock(&auxCache->globalLock);

					endRead = Util::ClockGetTime();
					//std::cout << "Read Time: "<< retValue->getName() << " "<<endRead-initRead<< std::endl;
				}
				break;
			}
		}
	}
	return retValue;
}

void RegionTemplate::setTimestamp(int timestamp) {
	this->timestamp = timestamp;
}

void RegionTemplate::setVersion(unsigned int version) {
	this->version = version;
}

// This function is partially implemented. It is not serializing all the data fields
int RegionTemplate::serialize(char* buff) {
	int serialized_bytes = 0;

	// number of chars in the name
	int nameSize = this->name.size();
	memcpy(buff+serialized_bytes, &nameSize, sizeof(int));
	serialized_bytes += sizeof(int);

	// pack the name itself
	memcpy(buff+serialized_bytes, this->name.c_str(), sizeof(char)*nameSize);
	serialized_bytes += nameSize * sizeof(char);

	// pack number of chars in the region template id
	nameSize = this->getId().size();
	memcpy(buff+serialized_bytes, &nameSize, sizeof(int));
	serialized_bytes += sizeof(int);

	// pack the id itself
	memcpy(buff+serialized_bytes, this->getId().c_str(), sizeof(char)*nameSize);
	serialized_bytes += nameSize * sizeof(char);

	// pack lazy
	memcpy(buff+serialized_bytes, &this->lazyRead, sizeof(bool));
	serialized_bytes += sizeof(bool);


	// pack the number of data regions in this template
	int regionSize = 0;
	std::map<std::string, std::list<DataRegion*> >::iterator it = this->templateRegions.begin();
	// walk through all data regions w/ different names
	for(; it != templateRegions.end(); it++){
		// Sum the number of data regions w/ the same name
		regionSize += (*it).second.size();
	}

	// pack the number of data regions in this template
	memcpy(buff+serialized_bytes, &regionSize, sizeof(int));
	serialized_bytes += sizeof(int);

	// pack each of the data regions
	it = this->templateRegions.begin();
	for(; it != templateRegions.end(); it++){
		// Here we have each entry of the map with the same name. However, there may be several
		// regions w/ the same name that differ in timestamp and version
		std::list<DataRegion*>::iterator listIt = (*it).second.begin();

		// Walk through all the data regions w/ the same name and get their sizes.
		for(; listIt != (*it).second.end(); listIt++){
			serialized_bytes += (*listIt)->serialize(buff+serialized_bytes);
		}
	}

	return serialized_bytes;
}

int RegionTemplate::deserialize(char* buff) {
	int deserialized_bytes = 0;

	// Extract the size of the RT name
	int nameSize = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes+=sizeof(int);

	// Extract name of the region template
	char rtName[nameSize+1];
	rtName[nameSize] = '\0';
	memcpy(rtName, buff+deserialized_bytes, sizeof(char)*nameSize);
	deserialized_bytes += sizeof(char)*nameSize;
	this->setName(rtName);

	// Extract the size of the RT id
	int idSize = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes+=sizeof(int);

	// Extract id of the region template
	char rtId[idSize+1];
	rtId[idSize] = '\0';
	memcpy(rtId, buff+deserialized_bytes, sizeof(char)*idSize);
	deserialized_bytes += sizeof(char)*idSize;
	this->setId(rtId);

	// extract lazy read variable
	bool isLazy;
	memcpy(&isLazy, buff+deserialized_bytes, sizeof(bool));
	deserialized_bytes += sizeof(bool);
	this->setLazyRead(isLazy);

	// Extract number of data regions
	int regionSize = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);
	std::cout << "\tnDataRegions deserialize:"<< regionSize << std::endl;
	// extract each of the data regions
	for(int i = 0; i < regionSize; i++){
		// create data region: assuming that it is a 2D. Might not be the case.
		DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
//		std::cout << "\t\t deserialized: before: " << deserialized_bytes << std::endl;
		deserialized_bytes += ddr2d->deserialize(buff+deserialized_bytes);
//		std::cout << "\t\t deserialized: after: " << deserialized_bytes << std::endl;
		this->insertDataRegion(ddr2d);
		ddr2d->setModified(false);

	}
	return deserialized_bytes;

}


int RegionTemplate::size() {
	int size = 0;

	// space (int) to store number of chars in the name
	int nameSize = this->name.size();
	size += sizeof(int);

	// space to store the chars in the name
	size += nameSize * sizeof(char);

	// space (int) to store number of char in the id
	size+=sizeof(int);

	// space to store char in the id
	size += this->getId().size() * sizeof(char);

	// pack the lazy read variable
	size += sizeof(bool);

	// pack the number of data regions in this template
	size += sizeof(int);

	// pack each of the data regions
	std::map<std::string, std::list<DataRegion*> >::iterator it = this->templateRegions.begin();
	for(; it != templateRegions.end(); it++){

		// Here we have each entry of the map with the same name. However, there may be several
		// regions w/ the same name that differ in timestamp and version
		std::list<DataRegion*>::iterator listIt = (*it).second.begin();

		// Walk through all the data regions w/ the same name and get their sizes.
		for(; listIt != (*it).second.end(); listIt++){
			size += (*listIt)->serializationSize();
		}
	}

	return size;
}


std::string RegionTemplate::getId() const {
	return id;
}

void RegionTemplate::setId(std::string id) {
	this->id = id;
}

int RegionTemplate::getNumDataRegions() {
	int retValue = 0;
	std::map<std::string, std::list<DataRegion*> >::iterator it = this->templateRegions.begin();

	// walk through data regions w/ same name and get size of each of those lists
	for(; it != this->templateRegions.end(); it++){
		retValue += (*it).second.size();
	}

	return retValue;
}

DataRegion* RegionTemplate::getDataRegion(int index) {
	DataRegion* dr = NULL;

	if(index >= 0 && index < this->getNumDataRegions()){
		// Clean up regions stored within this region templates
		std::map<std::string, std::list<DataRegion*> >::iterator it = this->templateRegions.begin();

		int baseIndex = 0;
//		std::cout << "getDR: " << index << std::endl;
		for(; it != this->templateRegions.end(); it++){
//			std::cout << "index: " << index << " baseIndex: "<< baseIndex << " (*it).second.size: "<< (*it).second.size() << std::endl;
			if(index <= baseIndex + (*it).second.size() -1){
				std::list<DataRegion*>::iterator listIt = (*it).second.begin();

				while(index != baseIndex){
//					std::cout << "baseIndex ++ " << std::endl;
					baseIndex++;
					listIt++;
				}
				dr = (*listIt);
				break;
			}else{
//				std::cout << "baseIndex += " << (*it).second.size() << std::endl;
				baseIndex += (*it).second.size();
			}
		}
	}
	return dr;
}


void RegionTemplate::print() {
	std::cout << "########## REGION TEMPLATE ###############" << std::endl;
	std::cout << "name: "<< this->getName() << " id: " << this->getId() << std::endl;

	std::map<std::string, std::list<DataRegion*> >::iterator it = this->templateRegions.begin();
	for(; it != this->templateRegions.end(); it++){
		// Here we have each entry of the map with the same name. However, there may be several
		// regions w/ the same name that differ in timestamp and version
		std::list<DataRegion*>::iterator listIt = (*it).second.begin();

		// Walk through all the data regions w/ the same name and print them
		for(; listIt != (*it).second.end(); listIt++){
			DataRegion *dr = (*listIt);
			dr->print();
	/*		std::cout << "\t\t name: "<< dr->getName() << std::endl;
			std::cout << "\t\t id: "<< dr->getId() << std::endl;
			std::cout << "\t\t version: "<< dr->getVersion() << std::endl;
			std::cout << "\t\t timestamp: "<< dr->getTimestamp() << std::endl;
			std::cout << "\t\t inType: "<< dr->getInputType() << std::endl;
			std::cout << "\t\t outType: "<< dr->getOutputType() << std::endl;*/
		}
	}

	std::cout << "########## END REGION TEMPLATE ###############" << std::endl;

}
// Get number of regions with a given name which differ in temporal component and version
int RegionTemplate::getDataRegionListSize(std::string name) {
	int retValue = 0;

	// friendly iterator to walk through the list of regions with same name (equals to parameter 'name')
	std::map<std::string, std::list<DataRegion*> >::iterator it;

	// search for required region within this region template
	it = templateRegions.find(name);

	// if we have found regions with that  name
	if(it != templateRegions.end()){
		// get the size of the list of regions w/ this name
		retValue = (*it).second.size();
	}
	return retValue;
}

bool RegionTemplate::isLazyRead() const {
	return lazyRead;
}

int RegionTemplate::getLocation() const {
	return location;
}

void RegionTemplate::setLocation(int location) {
	this->location = location;
}

void RegionTemplate::setLazyRead(bool lazyRead) {
	this->lazyRead = lazyRead;
}

void RegionTemplate::setCache(Cache* cache) {
	this->cachePtr = cache;
}

Cache* RegionTemplate::getCache(void) {
	return this->cachePtr;
}

bool RegionTemplate::deleteDataRegion(std::string drName, std::string drId,
		int timestamp, int version) {
	// Did not find the region requested by default
	DataRegion *retValue = NULL;

	// friendly iterator to walk through the list of regions with same name (equals to parameter 'name')
	std::map<std::string, std::list<DataRegion*> >::iterator it;

	// search for required region within this region template
	it = templateRegions.find(drName);

	// if we have found regions with that  name
	if(it != templateRegions.end()){
		// try to find a region with same version and timestamp. This should be a happy matching
		std::list<DataRegion*>::iterator listIt = (*it).second.begin();

		// check if a region with same version and timestamp exist
		for(; listIt != (*it).second.end(); listIt++){

			// if found the specific region. If drId is empty, any id is accepted
			if( (drId.compare((*listIt)->getId()) == 0 || drId.empty())&&
				timestamp == (*listIt)->getTimestamp() &&
				version == (*listIt)->getVersion() ){
				// set the return value as to that region
				retValue = (*listIt);
				// if required data region has not been loaded
				if(retValue->empty() && (this->getLocation() == PipelineComponentBase::WORKER_SIDE)){

					Cache* auxCache = this->getCache();

					//pthread_mutex_lock(&auxCache->globalLock);
					if(auxCache != NULL){
						#ifdef DEBUG
						std::cout << "auxCache->deleteDr: " << this->getName() << ":"<< this->getId() << ":"<< retValue->getName() << ":"<<  retValue->getId() << ":"<<  retValue->getTimestamp() << ":"<<  retValue->getVersion() << ":"<<  true << ":"<<  auxCache->getWorkerId() << ":"<<  retValue->getWorkerId() << std::endl;
						#endif

						auxCache->deleteDR(this->getName(), this->getId(), retValue->getName(), retValue->getId(), retValue->getTimestamp(), retValue->getVersion());

					}else{
						std::cout << "RT::getDataRegion: CACHE is NULL. rtName: "<< this->getName() << std::endl;

					}

					//pthread_mutex_unlock(&auxCache->globalLock);
				}
				break;
			}
		}
	}
	return true;
}
