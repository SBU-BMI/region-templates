#include "DataRegion.h"

DataRegion::DataRegion() {
	this->version = 0;
	this->timestamp = 0;
	this->resolution = 0;
	this->elementsType = -1;
	this->regionType = -1;

	this->isAppInput = false;
	this->inputType = DataSourceType::FILE_SYSTEM;
	this->outputType = DataSourceType::FILE_SYSTEM;

	this->storageLevel = -1;

	this->outputExtension = DataRegion::PBM;

	this->cacheLevel = -1;
	this->cacheType = -1;
	this->workerId = -1;
	this->cachedDataSize = 0;
	this->modified = false;

	this->ROI.setLb(Point(0,0,0));
	this->ROI.setUb(Point(0,0,0));

	this->bb.setLb(Point(0,0,0));
	this->bb.setUb(Point(0,0,0));

	this->isSvs = false;
}

DataRegion::~DataRegion() {

}

int DataRegion::getType() const {
	return regionType;
}

const BoundingBox& DataRegion::getBb() const {
	return bb;
}

void DataRegion::setBb(const BoundingBox& bb) {
	this->bb = bb;
}

void DataRegion::setName(std::string name) {
	this->name = name;
}

int DataRegion::getResolution() const {
	return resolution;
}

void DataRegion::setResolution(int resolution) {
	this->resolution = resolution;
}

int DataRegion::getTimestamp() const {
	return timestamp;
}

void DataRegion::setTimestamp(int timestamp) {
	this->timestamp = timestamp;
}

int DataRegion::getVersion() const {
	return version;
}

std::string DataRegion::getName() const {
	return name;
}

void DataRegion::setVersion(int version) {
	this->version = version;
}

void DataRegion::setType(int type) {
	this->regionType = type;
}

void DataRegion::setSvs() {
	this->isSvs = true;
}

bool DataRegion::write() {
	return false;
}

bool DataRegion::instantiateRegion() {
	return false;
}

int DataRegion::serialize(char* buff) {
	int serialized_bytes = 0;

	// Data region data
	int type = this->getType();
	memcpy(buff+serialized_bytes, &type, sizeof(int));
	serialized_bytes += sizeof(int);

	// size of the name
	int nameSize = this->getName().size();
	memcpy(buff+serialized_bytes, &nameSize, sizeof(int));
	serialized_bytes += sizeof(int);

//	std::cout << "\tdataRegion name Size: "<< nameSize << std::endl;
	// pack the name itself
	memcpy(buff+serialized_bytes, this->getName().c_str(), sizeof(char)*nameSize);
	serialized_bytes += nameSize * sizeof(char);

	// Input data source type
	memcpy(buff+serialized_bytes, &this->inputType, sizeof(int));
	serialized_bytes += sizeof(int);

	// Output data source type
	memcpy(buff+serialized_bytes, &this->outputType, sizeof(int));
	serialized_bytes += sizeof(int);

	// Output data source extension
	memcpy(buff+serialized_bytes, &this->outputExtension, sizeof(int));
	serialized_bytes += sizeof(int);

	// Data region timestamp
	memcpy(buff+serialized_bytes, &this->timestamp, sizeof(int));
	serialized_bytes += sizeof(int);

	// Data region version
	memcpy(buff+serialized_bytes, &this->version, sizeof(int));
	serialized_bytes += sizeof(int);

	int dataRegionIdSize = this->getId().size();

	// Size of the region id name
	memcpy(buff+serialized_bytes, &dataRegionIdSize, sizeof(int));
	serialized_bytes += sizeof(int);

	// pack the id itself
	memcpy(buff+serialized_bytes, this->getId().c_str(), sizeof(char)*dataRegionIdSize);
	serialized_bytes += dataRegionIdSize * sizeof(char);

	// pack isAppInput
	memcpy(buff+serialized_bytes, &this->isAppInput, sizeof(bool));
	serialized_bytes += sizeof(bool);

	int inputFileNameSize = (int) this->getInputFileName().size();
	// size of inputFileName
	memcpy(buff+serialized_bytes, &inputFileNameSize, sizeof(int));
	serialized_bytes += sizeof(int);

	// to store inputFileName
	memcpy(buff+serialized_bytes, this->getInputFileName().c_str(), sizeof(char)*inputFileNameSize);
	serialized_bytes += inputFileNameSize * sizeof(char);

	// pack data region bounding box
	BoundingBox bb = this->getBb();
	serialized_bytes += bb.serialize(buff+serialized_bytes);

	// pack data region ROI
	bb = this->getROI();
	serialized_bytes += bb.serialize(buff+serialized_bytes);

	// pack isSvs
	memcpy(buff+serialized_bytes, &this->isSvs, sizeof(bool));
	serialized_bytes += sizeof(bool);

	// pack cache leve in which data is stored
	memcpy(buff+serialized_bytes, &this->cacheLevel, sizeof(int));
	serialized_bytes += sizeof(int);

	// type of cache : global, local
	memcpy(buff+serialized_bytes, &this->cacheType, sizeof(int));
	serialized_bytes += sizeof(int);

	// id (MPI) of th worker in which the data is cached.
	memcpy(buff+serialized_bytes, &this->workerId, sizeof(int));
	serialized_bytes +=sizeof(int);

	// size of the data stored w/ this data region
	memcpy(buff+serialized_bytes, &this->cachedDataSize, sizeof(long));
	serialized_bytes +=sizeof(long);

	// pack the number of chunks in which the data
	// regions is divided
	int nChunks = bb2Id.size();
	memcpy(buff+serialized_bytes, &nChunks, sizeof(int));
	serialized_bytes += sizeof(int);

	std::vector<std::pair<BoundingBox, std::string> >::iterator bb2IdIt = bb2Id.begin();
	while(bb2IdIt != bb2Id.end()){
		// size of the bounding box
		serialized_bytes += (*bb2IdIt).first.serialize(buff+serialized_bytes);

		// integer to store string size
		int idSize = (*bb2IdIt).second.size();
		memcpy(buff+serialized_bytes, &idSize, sizeof(int));
		serialized_bytes += sizeof(int);

		// size of the string id
		memcpy(buff+serialized_bytes, (*bb2IdIt).second.c_str(), (*bb2IdIt).second.size()*sizeof(char));
		serialized_bytes += (*bb2IdIt).second.size() * sizeof(char);

		bb2IdIt++;
	}

	return serialized_bytes;
}

int DataRegion::deserialize(char* buff) {
	int deserialized_bytes = 0;

	// extract data region type
	int region_type =((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract the size of the DR name
	int nameSize = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes+=sizeof(int);
//	std::cout << "\tdataRegion name Size: "<< nameSize << std::endl;

	// extract name of the data region
	char drName[nameSize+1];
	drName[nameSize] = '\0';
	memcpy(drName, buff+deserialized_bytes, sizeof(char)*nameSize);
	deserialized_bytes += sizeof(char)*nameSize;
	this->setName(drName);
//	std::cout << "\tdataRegion name : "<< drName << std::endl;

	// extract input data source type
	int ids_type =((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);
	this->setInputType(ids_type);

	// extract output data source type
	int ods_type =((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);
	this->setOutputType(ods_type);

	// extract output data source type
	int ods_extension =((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);
	this->setOutputExtension(ods_extension);

	// extract data region timestamp
	int timeStamp =((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);
	this->setTimestamp(timeStamp);

	// extract data region version
	int version = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);
	this->setVersion(version);

	int id_size = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	char idName[id_size+1];
	idName[id_size] = '\0';
	memcpy(idName, buff+deserialized_bytes, sizeof(char)*id_size);
	deserialized_bytes += sizeof(char)*id_size;
	this->setId(idName);

	// unpack
	bool isAppInput;
	memcpy(&isAppInput, buff+deserialized_bytes, sizeof(bool));
	deserialized_bytes += sizeof(bool);
	this->setIsAppInput(isAppInput);

	int inputFileSize = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	char inputFile[inputFileSize+1];
	inputFile[inputFileSize] = '\0';
	memcpy(inputFile, buff+deserialized_bytes, sizeof(char)*inputFileSize);
	deserialized_bytes += sizeof(char)*inputFileSize;
	this->setInputFileName(inputFile);

	BoundingBox bb;
	deserialized_bytes += bb.deserialize(buff+deserialized_bytes);
	this->setBb(bb);

	BoundingBox ROI;
	deserialized_bytes += ROI.deserialize(buff+deserialized_bytes);
	this->setROI(ROI);

	// unpack
	bool isSvs;
	memcpy(&isSvs, buff+deserialized_bytes, sizeof(bool));
	deserialized_bytes += sizeof(bool);
	this->isSvs = isSvs;

	// unpack cache level
	this->setCacheLevel(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// unpack cache type
	this->setCacheType(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// unpack worker id
	this->setWorkerId(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// unpack cachedDataSize
	this->setCachedDataSize(((long*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(long);

	// unpack the number of chunks in which the data
	// regions is divided
	// extract output data source type
	int nChunks =((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	for(int i = 0; i < nChunks; i++){
		BoundingBox bb;
		deserialized_bytes += bb.deserialize(buff+deserialized_bytes);

		int idSize = ((int*)(buff+deserialized_bytes))[0];
		deserialized_bytes += sizeof(int);

		char id[idSize+1];
		id[idSize] = '\0';
		memcpy(id, buff+deserialized_bytes, sizeof(char) * idSize);
		deserialized_bytes += sizeof(char) * idSize;
		// insert data chunk to data region
		this->insertBB2IdElement(bb, id);
	}

	return deserialized_bytes;
}

int DataRegion::getInputType() const {
	return inputType;
}

void DataRegion::setInputType(int inputType) {
	this->inputType = inputType;
}

int DataRegion::getOutputType() const {
	return outputType;
}

std::string DataRegion::getId() const {
	return id;
}

void DataRegion::setId(std::string id) {
	this->id = id;
}

bool DataRegion::empty() {
	return false;
}

void DataRegion::setOutputType(int outputType) {
	this->outputType = outputType;
}

int DataRegion::serializationSize() {
	int size_bytes = 0;

	// data region type
	size_bytes += sizeof(int);

	// size of the DR name
	size_bytes +=sizeof(int);

	// name of the data region
	size_bytes += this->getName().size() * sizeof(char);

	// input data source type
	size_bytes += sizeof(int);

	// output data source type
	size_bytes += sizeof(int);

	// output data source extension
	size_bytes += sizeof(int);

	// data region timestamp
	size_bytes += sizeof(int);

	// data region version
	size_bytes += sizeof(int);

	// size of id
	size_bytes += sizeof(int);

	// space to store id
	size_bytes += this->getId().size() * sizeof(char);

	// to store isAppInput
	size_bytes += sizeof(bool);

	// size of inputFileName
	size_bytes += sizeof(int);

	// to store inputFileName
	size_bytes += this->getInputFileName().size() * sizeof(char);

	// space to store bounding box
	BoundingBox bb = this->getBb();
	size_bytes += bb.size();

	// space to store ROI
	bb = this->getROI();
	size_bytes += bb.size();

	// to store isSvs
	size_bytes += sizeof(bool);

	// size of cache level
	size_bytes += sizeof(int);

	// size of cache type
	size_bytes += sizeof(int);

	// size of worker id
	size_bytes += sizeof(int);

	// size of cachedDataSize
	size_bytes += sizeof(long);

	// pack the number of chunks in which the data
	// regions is divided
	size_bytes += sizeof(int);

	std::vector<std::pair<BoundingBox, std::string> >::iterator bb2IdIt = bb2Id.begin();
	while(bb2IdIt != bb2Id.end()){
		// size of the bounding box
		size_bytes += (*bb2IdIt).first.size();

		// integer to store string size
		size_bytes += sizeof(int);

		// size of the string id
		size_bytes += (*bb2IdIt).second.size() * sizeof(char);

		bb2IdIt++;
	}

	return size_bytes;
}

const BoundingBox& DataRegion::getROI() const {
	return ROI;
}

void DataRegion::setROI(const BoundingBox& roi) {
	ROI = roi;
}

void DataRegion::insertBB2IdElement(BoundingBox bb, std::string id) {
	bb2Id.push_back(std::make_pair(bb, id));
}

void DataRegion::print() {
	std::cout << "###### PRINT DATA REGION ######" << std::endl;
	std::cout << "\tName: " << this->getName() << std::endl;
	std::cout << "\tId: "<< this->getId() << std::endl;
	std::cout << "\tinputFileName: " << this->getInputFileName() << std::endl;
	std::cout << "\tTimestamp: "<< this->getTimestamp() << std::endl;
	std::cout << "\tVersion: "<< this->getVersion() << std::endl;
	std::cout << "\tDRType: "<< this->getType() << std::endl;
	std::cout << "\tisAppInput?:" << this->getIsAppInput() << std::endl;
	std::cout << "\tInType: " << this->getInputType() << std::endl;
	std::cout << "\tOutType: " << this->getOutputType() << std::endl;
	std::cout << "\tWorkerId: "<< this->getWorkerId()<< std::endl;
	std::cout << "\tCacheLevel: " << this->getCacheLevel() << std::endl;
	std::cout << "\tCachedDataSize: " << this->getCachedDataSize() << std::endl;

	this->getBb().print();
	std::cout << std::endl;
	std::cout << "\t ROI: ";
	this->getROI().print();
	std::cout << std::endl;
	std::cout << "\tNumber of data chunks: " << bb2Id.size() << std::endl;

	std::vector<std::pair<BoundingBox, std::string> >::iterator bb2IdIt = bb2Id.begin();
	while(bb2IdIt != bb2Id.end()){

		std::cout << "\t\t";
		(*bb2IdIt).first.print();
		std::cout << (*bb2IdIt).second << std::endl;

		bb2IdIt++;
	}
	std::cout << "###### END PRINT DATA REGION ######" << std::endl;
}

std::pair<BoundingBox, std::string> DataRegion::getBB2IdElement(int index) {
	if(index < 0 || index > this->bb2Id.size()){
		std::cout << "Trying to access chunked data out of bounds. Index: "<<index << ", but number of BBs: " << bb2Id.size();
		BoundingBox emptyBB;
		std::string emptyStr;
		return std::make_pair(emptyBB, emptyStr);
	}else{
		return bb2Id[index];
	}
}

int DataRegion::getBB2IdSize() {
	return this->bb2Id.size();
}

int DataRegion::getOutputExtension() const {
	return outputExtension;
}

void DataRegion::setOutputExtension(int outputExtension) {
	this->outputExtension = outputExtension;
}

DataRegion* DataRegion::clone(bool copyData) {
	return NULL;
}

long DataRegion::getDataSize() {
	return -1;
}

std::string DataRegion::getInputFileName() const {
	return inputFileName;
}

void DataRegion::setInputFileName(std::string inputFileName) {
	this->inputFileName = inputFileName;
}

bool DataRegion::getIsAppInput() const {
	return isAppInput;
}

int DataRegion::getCacheLevel() const {
	return cacheLevel;
}

void DataRegion::setCacheLevel(int cacheLevel) {
	this->cacheLevel = cacheLevel;
}

int DataRegion::getCacheType() const {
	return cacheType;
}

void DataRegion::setCacheType(int cacheType) {
	this->cacheType = cacheType;
}

int DataRegion::getWorkerId() const {
	return workerId;
}

void DataRegion::setWorkerId(int workerId) {
	this->workerId = workerId;
}

long DataRegion::getCachedDataSize() const {
	return cachedDataSize;
}

bool DataRegion::isModified() const {
	return modified;
}

void DataRegion::setModified(bool modified) {
	this->modified = modified;
}

void DataRegion::setCachedDataSize(long cachedDataSize) {
	this->cachedDataSize = cachedDataSize;
}

void DataRegion::setIsAppInput(bool isAppInput) {
	this->isAppInput = isAppInput;
}

int DataRegion::getStorageLevel() const {
	return storageLevel;
}

void DataRegion::setStorageLevel(int storageLevel) {
	this->storageLevel = storageLevel;
}
