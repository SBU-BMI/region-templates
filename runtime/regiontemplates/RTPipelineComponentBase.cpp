/*
 * RTPipelineComponentBase.cpp
 *
 *  Created on: Feb 13, 2013
 *      Author: george
 */

#include "RTPipelineComponentBase.h"

RTPipelineComponentBase::RTPipelineComponentBase() {
	this->setType(PipelineComponentBase::RT_COMPONENT_BASE);
	this->cache = NULL;
}

RTPipelineComponentBase::~RTPipelineComponentBase() {
	// Delete associated region templates: this is wrong. Other components also access
	// the same region templates
	if(this->getLocation() == PipelineComponentBase::WORKER_SIDE){
#ifdef DEBUG
		std::cout << "~RTPipelineComponentBase: nRegionTemplates: " << this->regionTemplates.size() << std::endl;
#endif
		std::map<std::string, RegionTemplate*>::iterator rtIt =  this->regionTemplates.begin();
		for(; rtIt != this->regionTemplates.end(); rtIt++){
			RegionTemplate *rt = rtIt->second;
			delete rt;
		}
	}
}

int RTPipelineComponentBase::serialize(char* buff) {
//	std::cout << "\t THIS IS RT_PIPELINE_COMPONENT:serialize" << std::endl;
	int serialized_bytes = PipelineComponentBase::serialize(buff);

	// pack number of input data regions
	int num_input_regions = this->input_data_regions.size();
	memcpy(buff+serialized_bytes, &num_input_regions, sizeof(int));
	serialized_bytes += sizeof(int);

	std::set<std::pair<std::string, std::string> >::iterator irIt = input_data_regions.begin();

	for(int i = 0; i < num_input_regions; i++, irIt++){
		// name of the region template
		std::string rtName = irIt->first;

		// pack string size
		int rtNameSize  = rtName.size();
		memcpy(buff+serialized_bytes, &rtNameSize, sizeof(int));
		serialized_bytes += sizeof(int);

		// pack string
		memcpy(buff+serialized_bytes, rtName.c_str(), sizeof(char)*rtNameSize);
		serialized_bytes += sizeof(char)*rtNameSize;

		// name of the data region used within this region template
		std::string drName = irIt->second;

		// pack string size
		int drNameSize  = drName.size();
		memcpy(buff+serialized_bytes, &drNameSize, sizeof(int));
		serialized_bytes += sizeof(int);

		// pack string
		memcpy(buff+serialized_bytes, drName.c_str(), sizeof(char)*drNameSize);
		serialized_bytes += sizeof(char)*drNameSize;
	}

	int num_output_regions = this->output_data_regions.size();
	memcpy(buff+serialized_bytes, &num_output_regions, sizeof(int));
	serialized_bytes += sizeof(int);

	std::set<std::pair<std::string, std::string> >::iterator orIt = output_data_regions.begin();
	for(int i = 0; i < num_output_regions; i++, orIt){
		// name of the region template
		std::string rtName = orIt->first;

		// pack string size
		int rtNameSize  = rtName.size();
		memcpy(buff+serialized_bytes, &rtNameSize, sizeof(int));
		serialized_bytes += sizeof(int);

		// pack string
		memcpy(buff+serialized_bytes, rtName.c_str(), sizeof(char)*rtNameSize);
		serialized_bytes += sizeof(char)*rtNameSize;

		// name of the data region used within this region template
		std::string drName = orIt->second;

		// pack string size
		int drNameSize  = drName.size();
		memcpy(buff+serialized_bytes, &drNameSize, sizeof(int));
		serialized_bytes += sizeof(int);

		// pack string
		memcpy(buff+serialized_bytes, drName.c_str(), sizeof(char)*drNameSize);
		serialized_bytes += sizeof(char)*drNameSize;

	}

	// Get the number of region template instances used by this pipeline
	int nRegionTemplateInsts = this->regionTemplates.size();

//	std::cout << "Serialize numRTInsntaces:" << nRegionTemplateInsts << std::endl;

	// Pack this information
	memcpy(buff+serialized_bytes, &nRegionTemplateInsts, sizeof(int));
	serialized_bytes += sizeof(int);

	std::map<std::string, RegionTemplate*>::iterator it = this->regionTemplates.begin();

	for(int i = 0; i < nRegionTemplateInsts; i++){
		serialized_bytes += it->second->serialize(buff+serialized_bytes);
//		it->second->print();
		it++;
	}

	return serialized_bytes;
}

int RTPipelineComponentBase::deserialize(char* buff) {
//	std::cout << "\t THIS IS RT_PIPELINE_COMPONENT:deserialize" << std::endl;
	int deserialized_bytes = PipelineComponentBase::deserialize(buff);

	// unpack number of input data regions
	int num_input_regions;
	memcpy(&num_input_regions, buff+deserialized_bytes, sizeof(int));
	deserialized_bytes += sizeof(int);

	for(int i = 0; i < num_input_regions; i++){
		// name of the region template
		int rtNameSize;
//		std::string rtName; = irIt->first;

		// unpack string size
		memcpy(&rtNameSize, buff+deserialized_bytes, sizeof(int));
		deserialized_bytes += sizeof(int);

		char rtName[rtNameSize+1];
		rtName[rtNameSize] = '\0';

		// unpack string
		memcpy(rtName, buff+deserialized_bytes, sizeof(char)*rtNameSize);
		deserialized_bytes += sizeof(char)*rtNameSize;


		// unpack string size
		int drNameSize;
		memcpy(&drNameSize, buff+deserialized_bytes,  sizeof(int));
		deserialized_bytes += sizeof(int);

		char drName[drNameSize+1];
		drName[drNameSize] = '\0';

		// unpack string
		memcpy(drName, buff+deserialized_bytes, sizeof(char)*drNameSize);
		deserialized_bytes += sizeof(char)*drNameSize;
		this->addInputOutputDataRegion(rtName, drName, RTPipelineComponentBase::INPUT);
	}

	int num_output_regions;
	memcpy(&num_output_regions, buff+deserialized_bytes, sizeof(int));
	deserialized_bytes += sizeof(int);

	for(int i = 0; i < num_output_regions; i++){

		// unpack string size
		int rtNameSize;
		memcpy(&rtNameSize, buff+deserialized_bytes, sizeof(int));
		deserialized_bytes += sizeof(int);

		char rtName[rtNameSize+1];
		rtName[rtNameSize] = '\0';

		// unpack string
		memcpy(rtName, buff+deserialized_bytes, sizeof(char)*rtNameSize);
		deserialized_bytes += sizeof(char)*rtNameSize;


		// unpack string size
		int drNameSize;
		memcpy(&drNameSize, buff+deserialized_bytes, sizeof(int));
		deserialized_bytes += sizeof(int);

		char drName[drNameSize+1];
		drName[drNameSize] = '\0';

		// unpack string
		memcpy(drName, buff+deserialized_bytes, sizeof(char)*drNameSize);
		deserialized_bytes += sizeof(char)*drNameSize;
		this->addInputOutputDataRegion(rtName, drName, RTPipelineComponentBase::OUTPUT);
	}

	int nRegionTemplateInsts = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes +=sizeof(int);
//	std::cout << "Deserialize numRTInsntaces:" << nRegionTemplateInsts << std::endl;

	for(int i = 0; i < nRegionTemplateInsts; i++){
		RegionTemplate *rt = new RegionTemplate();
		deserialized_bytes += rt->deserialize(buff+deserialized_bytes);
		this->addRegionTemplateInstance(rt, rt->getName());
//		std::cout << "Deserialize: add RT: "<<rt->getName() << std::endl;
	}
//	std::cout << "\t THIS IS END RT_PIPELINE_COMPONENT:deserialize" << std::endl;

	return deserialized_bytes;
}


PipelineComponentBase* RTPipelineComponentBase::clone() {
	RTPipelineComponentBase* retValue = new RTPipelineComponentBase();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete buff;
	return retValue;
}

RegionTemplate* RTPipelineComponentBase::getRegionTemplateInstance(std::string dataRegionName) {
	RegionTemplate *retRegionTemplate = NULL;

	// std::cout << "==========================================" << std::endl;
	// std::cout << "RTs:" << std::endl;
	// for (std::pair<std::string, RegionTemplate*> p : this->regionTemplates) {
	// 	std::cout << "name: " << p.first << std::endl;
	// }
	// std::cout << "==========================================" << std::endl;

	std::map<std::string, RegionTemplate*>::iterator it;
	it=this->regionTemplates.find(dataRegionName);
	if(it != this->regionTemplates.end()){
		retRegionTemplate = it->second;
	}

	// std::cout << "finding " << dataRegionName << " = " << retRegionTemplate << std::endl;
	return retRegionTemplate;
}

int RTPipelineComponentBase::addRegionTemplateInstance(RegionTemplate* rt, std::string name) {
	// set cache Ptr Value
	if(rt!=NULL){
		rt->setCache(this->getCache());
		rt->setLocation(this->getLocation());
	}
	this->regionTemplates.insert(std::make_pair(name, rt));
	return 1;
}

// NOT USED/USABLE
int RTPipelineComponentBase::instantiateRegionTemplates() {
#ifdef DEBUG
	std::cout << "instantiateRegionTemplates..Number of input data regions" << this->input_data_regions.size() << std::endl;
	std::cout << "instantiateRegionTemplates..Number of region templates: "<< this->regionTemplates.size()<<std::endl;
#endif
//	DataRegionFactory drf;
	std::set<std::pair<std::string, std::string> >::iterator inputDataRegionsIt = this->input_data_regions.begin();

	long long initStage = Util::ClockGetTime();

	// walk through input data regions/region templates and instantiate them
	for(; inputDataRegionsIt != this->input_data_regions.end(); inputDataRegionsIt++){
		string rtName = inputDataRegionsIt->first;
		string drName = inputDataRegionsIt->second;

		RegionTemplate * rt = this->getRegionTemplateInstance(rtName);
		if(rt != NULL){
			//std::map<std::string, std::list<DataRegion*> > templateRegions;
			std::map<std::string, std::list<DataRegion*> >::iterator drIt;

			// search for required region within this region template
			drIt = rt->templateRegions.find(drName);

			// if we have found regions with that  name
			if(drIt != rt->templateRegions.end()){
				// Iterate over all the data regions, regardless of timestamp and version
				std::list<DataRegion*>::iterator listIt = (*drIt).second.begin();
				for(; listIt != (*drIt).second.end(); listIt++){
					DataRegion *dr = (*listIt);
#ifdef DEBUG
					std::cout << "Instantiate data region: "<< rtName << ":" << drName << " timestamp: " << dr->getTimestamp() <<std::endl;
#endif
////					drf.instantiateDataRegion(dr);
//					// if data region is not local, load it to local cache.
//					if(dr->getWorkerId() != this->cache->getWorkerId()){
//						this->cache->get
//					}
				}

			}
		}else{
			std::cout << "Warning: could not find input regionTemplate " << rtName << std::endl;
		}
	}

	long long endStage = Util::ClockGetTime();
#ifdef DEBUG
	std::cout << "\t\treadingRTTime: "<< endStage-initStage << std::endl;
#endif
	return 0;
}

int RTPipelineComponentBase::getNumRegionTemplates() {
	return this->regionTemplates.size();
}

RegionTemplate* RTPipelineComponentBase::getRegionTemplateInstance(int index) {
	RegionTemplate* rt = NULL;
	if(index >= 0 && index < this->getNumRegionTemplates()){
		std::map<std::string, RegionTemplate*>::iterator itRT = this->regionTemplates.begin();
		for(int i =0; i < index; i++){
			itRT++;
		}
		rt = itRT->second;
	}
	return rt;
}

void RTPipelineComponentBase::updateRegionTemplateInfo(RegionTemplate* rt) {
	// there are two cases:
	// 1) the data regions exist or not
	RegionTemplate* curRt = this->getRegionTemplateInstance(rt->getName());
#ifdef DEBUG
	std::cout << "BEFORE UPDATE" << std::endl;
	if(curRt != NULL) curRt->print();
#endif

	if(curRt != NULL){
#ifdef DEBUG
		std::cout << "NUMDR: " << rt->getNumDataRegions() << std::endl;
#endif
		for(int i = 0; i < rt->getNumDataRegions(); i++){

			DataRegion* dr = rt->getDataRegion(i);
			DataRegion* drCur = curRt->getDataRegion(dr->getName(), dr->getId(), dr->getTimestamp(), dr->getVersion());
			if(drCur == NULL){
				// create data region;
				DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
				ddr2d->setName(dr->getName());

				drCur = ddr2d;
				drCur->setTimestamp(dr->getTimestamp());
				drCur->setVersion(dr->getVersion());
				drCur->setOutputExtension(dr->getOutputExtension());
				drCur->setInputFileName(dr->getInputFileName());
				curRt->insertDataRegion(ddr2d);

			}
			// update drCur information with received dr.
			drCur->setInputType(dr->getInputType());
			drCur->setOutputType(dr->getOutputType());
			drCur->setId(dr->getId());
			drCur->setInputFileName(dr->getInputFileName());
			drCur->setOutputExtension(dr->getOutputExtension());
			drCur->setWorkerId(dr->getWorkerId());
			drCur->setCacheLevel(dr->getCacheLevel());
			drCur->setCacheType(dr->getCacheType());
			drCur->setCachedDataSize(dr->getCachedDataSize());
#ifdef DEBUG
			std::cout << "CompName: "<< this->getComponentName() <<" Updating region template: "<< rt->getName() << " data region: "
					<< dr->getName()<<" dr->id: "<< drCur->getId() << " dr->timestamp: "<<dr->getTimestamp() << "dr->version: "<<dr->getVersion()
					<< " componentId: "<< this->getId()<< std::endl;
#endif

		}
	}else{
		std::cout << "Warning: failed to update region template: "<< rt->getName() << std::endl;
	}
#ifdef DEBUG
	std::cout << "AFTER UPDATE" << std::endl;
	if(curRt != NULL) curRt->print();
	std::cout << "#########" << std::endl;
#endif

}

// every time you change location, make sure region templates are updated
void RTPipelineComponentBase::setLocation(int location){
	this->location = location;
	for(int i=0; i < this->getNumRegionTemplates(); i++){
		RegionTemplate* aux = this->getRegionTemplateInstance(i);
		aux->setLocation(this->getLocation());;
	}
}

void RTPipelineComponentBase::addInputOutputDataRegion(
		std::string regionTemplateName, std::string dataRegionName, int type) {
	switch(type){
		case RTPipelineComponentBase::INPUT:
			this->input_data_regions.insert(std::make_pair<std::string, std::string>((std::string)regionTemplateName, (std::string)dataRegionName));
			break;
		case RTPipelineComponentBase::OUTPUT:

			this->output_data_regions.insert(std::make_pair<std::string, std::string>((std::string)regionTemplateName, (std::string)dataRegionName));
			break;

		case RTPipelineComponentBase::INPUT_OUTPUT:
			this->input_data_regions.insert(std::make_pair<std::string, std::string>((std::string)regionTemplateName, (std::string)dataRegionName));
			this->output_data_regions.insert(std::make_pair<std::string, std::string>((std::string)regionTemplateName, (std::string)dataRegionName));
			break;
		default:
			std::cout << "Error: unknown data region relation type (input, ouput,etc): "<< type << std::endl;
			break;
	}
}

int RTPipelineComponentBase::stageRegionTemplates() {
#ifdef DEBUG
	std::cout << "Number of output data regions" << this->output_data_regions.size() << std::endl;
#endif
	long long initStage = Util::ClockGetTime();

	// Check if the region templates need to be staged. if any data region is null, it is not staged.
	std::set<std::pair<std::string, std::string> >::iterator outputDataRegionsIt = this->output_data_regions.begin();

	DataRegionFactory drf;
	outputDataRegionsIt = this->output_data_regions.begin();

	// walk through output data regions/region templates and stage them
	for(; outputDataRegionsIt != this->output_data_regions.end(); outputDataRegionsIt++){
		string rtName = outputDataRegionsIt->first;
		string drName = outputDataRegionsIt->second;

		// retrieve region templates with referred name
		RegionTemplate * rt = this->getRegionTemplateInstance(rtName);

		// check if region template is not NULL and proceed to data staging
		if(rt != NULL){
			// friendly iterator to walk through the list of regions with same name (equals to parameter 'drName')
			std::map<std::string, std::list<DataRegion*> >::iterator it = rt->templateRegions.find(drName);;

			// if we have found regions with that name
			if(it != rt->templateRegions.end()){
#ifdef DEBUG
				std::cout << "CallBack: #drs "<< rt->templateRegions.size()<<std::endl;
#endif
				// Walk through all data regions with given name and stage them
				std::list<DataRegion*>::iterator listIt = (*it).second.begin();
				for(; listIt != (*it).second.end(); listIt++){
					DataRegion *dr = (*listIt);
					// if data region is not empty, stage it
					//if(!dr->empty()){
					Cache *auxCache = rt->getCache();

					//pthread_mutex_lock(&auxCache->globalLock);
					if(auxCache != NULL && dr->isModified()){
						rt->getCache()->insertDR(rt->getName(), rt->getId(), dr, true);
#ifdef DEBUG
						std::cout << "Stage data region into cache: "<< rtName << ":" << drName << " timestamp: "<< dr->getTimestamp() << " version:"<< dr->getVersion()<<" id: "<< dr->getId() <<std::endl;
#endif
					}

					//pthread_mutex_unlock(&auxCache->globalLock);
				}
			}else{
				std::cout << "Warning: could not find output regionTemplate:dataRegion " << rtName << ":" << drName << std::endl;
			}
		}else{
			std::cout << "Warning: could not find output regionTemplate " << rtName << std::endl;
		}
	}
	long long endStage = Util::ClockGetTime();
#ifdef DEBUG
	std::cout << "\t\tstagingRTTime: "<< endStage-initStage << std::endl;
#endif
	return 0;
}

int RTPipelineComponentBase::createIOTask() {
	// this was used to perform IO in background, but it makes IO operations less dynamic. Since
	// it need to read all data regions with a given name. There was no way to detetermine
	// specific version/timestamp
/*	TaskIO* taskIO = new TaskIO(this);
	int ioTaskID = taskIO->getId();
	this->executeTask(taskIO);
	return ioTaskID;*/
	return -1;
}

void RTPipelineComponentBase::setCache(Cache* cache) {
	//std::cout << "EXEC RTPIPE CACHE" << std::endl;
	this->cache = cache;

	// set cache ptr for all existing region templates
	for(int i = 0; i < this->getNumRegionTemplates(); i++){
		RegionTemplate* aux = getRegionTemplateInstance(i);
		//std::cout << "setCache: "<< aux->getName()<< " isCacheNull?: "<< (this->getCache()==NULL)<<std::endl;
		aux->setCache(this->cache);
	}
}

int RTPipelineComponentBase::size() {
	int size = PipelineComponentBase::size();

	// pack number of input data regions
	size += sizeof(int);

	int num_input_regions = input_data_regions.size();
	std::set<std::pair<std::string, std::string> >::iterator irIt = input_data_regions.begin();

	for(int i = 0; i < num_input_regions; i++, irIt++){
		// name of the region template
		std::string rtName = irIt->first;

		// pack string size
		int rtNameSize  = rtName.size();
		size += sizeof(int);

		// pack string
		size += sizeof(char)*rtNameSize;

		// name of the data region used within this region template
		std::string drName = irIt->second;

		// pack string size
		int drNameSize  = drName.size();
		size += sizeof(int);

		// pack string
		size += sizeof(char)*drNameSize;
	}

	int num_output_regions = this->output_data_regions.size();
	size += sizeof(int);

	std::set<std::pair<std::string, std::string> >::iterator orIt = output_data_regions.begin();
	for(int i = 0; i < num_output_regions; i++, orIt){
		// name of the region template
		std::string rtName = orIt->first;

		// pack string size
		int rtNameSize  = rtName.size();
		size += sizeof(int);

		// pack string
		size += sizeof(char)*rtNameSize;

		// name of the data region used within this region template
		std::string drName = orIt->second;

		// pack string size
		int drNameSize  = drName.size();
		size += sizeof(int);

		// pack string
		size += sizeof(char)*drNameSize;

	}

	// Store the number of region template instances used by this pipeline
	size += sizeof(int);

	// Get size of each region template
	std::map<std::string, RegionTemplate*>::iterator it = this->regionTemplates.begin();
	for(int i = 0; i < this->regionTemplates.size(); i++){
		size += it->second->size();
		it++;
	}

	return size;
}

long RTPipelineComponentBase::getAmountOfDataReuse(int workerId) {
	long dataReuse = 0;
	// for each region template within this component
	for(int j = 0; j < this->getNumRegionTemplates(); j++){
		RegionTemplate *rt = this->getRegionTemplateInstance(j);
		for(int i = 0; i < rt->getNumDataRegions(); i++){
				DataRegion* dr = rt->getDataRegion(i);
				if(dr->getWorkerId() == workerId){
					dataReuse+= dr->getCachedDataSize();

				}
		}
	}
	return dataReuse;
}
