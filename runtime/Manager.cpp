/*
 * Manager.cpp
 *
 *  Created on: Feb 15, 2012
 *      Author: george
 */

#include "Manager.h"

Manager::Manager(const MPI_Comm comm_world, const int manager_rank, 
	const int worker_size, const bool componentDataAwareSchedule, 
	const int queueType, const int cpuThreads, const int gpuThreads) {

	this->comm_world =comm_world;
	this->manager_rank = manager_rank;
	this->worker_size = worker_size;
	this->halideQueue = false;
	setFirstExecutionRound(true);
	this->componentDataAwareSchedule = componentDataAwareSchedule;
	for(int i = 0; i < worker_size; i++){
		std::vector<int> aux;
		this->compToSchedDataAwareSchedule.push_back(aux);
	}
	std::cout << "compToSchedSize: " << this->compToSchedDataAwareSchedule.size() << std::endl;

	switch (queueType) {
		case ExecEngineConstants::FCFS_QUEUE:
			componentsToExecute = new TasksQueueFCFS(1, 0);
			break;
		case ExecEngineConstants::HALIDE_TARGET_QUEUE:
			componentsToExecute = new TasksQueueHalide(cpuThreads, gpuThreads);
			this->halideQueue = true;
			break;
		case ExecEngineConstants::PRIORITY_QUEUE:
		default:
			componentsToExecute = new TasksQueuePriority(1, 0);
	}

	this->componentDependencies = new TrackDependencies();
}

Manager::~Manager() {
	if(componentsToExecute != NULL){
		delete componentsToExecute;
	}
	delete this->componentDependencies;
}

MPI_Comm Manager::getCommWorld() const
{
    return comm_world;
}

int Manager::getManagerRank() const
{
    return manager_rank;
}

int Manager::getWorkerSize() const
{
    return worker_size;
}

int Manager::finalizeExecution()
{
	MPI_Status status;
	int worker_id;
	char ready;


	/* tell everyone to quit */
	int active_workers = worker_size;
	std::cout << "finalizeExecution. worker_size: "<< worker_size << std::endl;
	std::vector<bool> finished;
	for(int i = 0; i < worker_size; i++)
		finished.push_back(false);

	while (active_workers > 0) {
		usleep(1000);
		
		int flag;
		MPI_Iprobe(MPI_ANY_SOURCE, MessageTag::TAG_CONTROL, this->comm_world, &flag, &status);
		if (flag) {
			// where is it coming from
			worker_id=status.MPI_SOURCE;
			MPI_Recv(&ready, 3, MPI_CHAR, worker_id, MessageTag::TAG_CONTROL, this->comm_world, MPI_STATUS_IGNORE);

			if (worker_id == manager_rank) continue;

			if(ready == MessageTag::WORKER_READY) {
				if(finished[worker_id] == false){
					MPI_Send(&MessageTag::MANAGER_FINISHED, 1, MPI_CHAR, worker_id, MessageTag::TAG_CONTROL, this->comm_world);
					printf("manager signal finished worker: %d manager_rank: %d\n", worker_id, manager_rank);
					--active_workers;
					finished[worker_id] = true;
				}
			}
		}
	}
	MPI_Barrier(this->comm_world);
	MPI_Finalize();
	return 0;
}

void Manager::checkConfiguration()
{

	MPI_Status status;
	int worker_id;
	char ready;
	bool correctInitialization = true;

	int active_workers = worker_size;

	// Listening from each worker whether it was initialized correctly
	while (active_workers > 0) {
		usleep(1000);

		int flag;
		MPI_Iprobe(MPI_ANY_SOURCE, MessageTag::TAG_CONTROL, this->comm_world, &flag, &status);
		if (flag) {
			// where is it coming from
			worker_id=status.MPI_SOURCE;
			bool curWorkerStatus;
			MPI_Recv(&curWorkerStatus, 1, MPI_C_BOOL, worker_id, MessageTag::TAG_CONTROL, this->comm_world, MPI_STATUS_IGNORE);
			if(curWorkerStatus == false){
				correctInitialization = false;
			}
			active_workers--;
		}
	}

	// Tell each worker whether to continue or quit the execution
	MPI_Bcast(&correctInitialization, 1 , MPI_C_BOOL, this->getManagerRank(), this->comm_world);

	if(correctInitialization==false){
		std::cout << "Quitting. Workers initialization failed. Possible due to errors loading the components library."<<std::endl;
		MPI_Finalize();
		exit(1);
	}
}


// send message to Worker requesting for a data region to be send to global storage
void Manager::sendDRInfoToWorkerForGlobalStorage(int worker_id, std::string rtName, std::string rtId,
		DataRegion* dr) {
	// tell worker that a DR should be moved to global storage
	MPI_Send(&MessageTag::CACHE_MOVE_DR_TO_GLOBAL_STORAGE, 1, MPI_CHAR, worker_id, MessageTag::TAG_CONTROL, this->comm_world);

	int msgSize = DataPack::packSize(rtName);
	msgSize += DataPack::packSize(rtId);
	msgSize += DataPack::packSize(dr->getName());
	msgSize += DataPack::packSize(dr->getId());
	msgSize += sizeof(int);
	msgSize += sizeof(int);

	char *msg = new char[msgSize];
	int serialized_bytes = 0;

	// unpack information about the DR that should be moved to global storage
	DataPack::pack(msg, serialized_bytes, rtName);
	DataPack::pack(msg, serialized_bytes, rtId);
	DataPack::pack(msg, serialized_bytes, dr->getName());
	DataPack::pack(msg, serialized_bytes, dr->getId());
	DataPack::pack(msg, serialized_bytes, dr->getTimestamp());
	DataPack::pack(msg, serialized_bytes, dr->getVersion());

	MPI_Send(msg, msgSize, MPI_CHAR, worker_id, MessageTag::TAG_CACHE_INFO, this->comm_world);

	delete msg;
}

void Manager::sendComponentInfoToWorker(int worker_id, PipelineComponentBase *pc)
{
#ifdef DEBUG
	std::cout << "Manager: Sending component id="<< pc->getId() <<std::endl;
#endif
	int comp_serialization_size = pc->size();
	char *buff = new char[comp_serialization_size];
	int used_serialization_size = pc->serialize(buff);
	assert(comp_serialization_size == used_serialization_size);

	if(pc->getType() == PipelineComponentBase::RT_COMPONENT_BASE){
		//std::cout << "This is a PipelineComponentBase::RT_COMPONENT_BASE" << std::endl;
		RTPipelineComponentBase *rtCurr = dynamic_cast<RTPipelineComponentBase*>(pc);
		// check whether there is any data region that is local but should be sent to a global storage
		// for each region template within this component
		Cache* cache = new Cache("rtconf.xml");
		cache->setWorkerId(this->manager_rank);
		rtCurr->setCache(cache);
		rtCurr->instantiateRegionTemplates();
		for(int j = 0; j < rtCurr->getNumRegionTemplates(); j++){
			RegionTemplate *rt = rtCurr->getRegionTemplateInstance(j);
			for(int i = 0; i < rt->getNumDataRegions(); i++){
				DataRegion *dr = rt->getDataRegion(i);
				// if data region is not an application input, is not on global storage,
				// or local with the work to which this component was assigned
				if(!dr->getIsAppInput() && dr->getCacheType() != Cache::GLOBAL && dr->getWorkerId() != worker_id){
					// if(dr->getWorkerId() != -1){
					// 	this->sendDRInfoToWorkerForGlobalStorage(dr->getWorkerId(), rt->getName(), rt->getId(), dr);
					// 	std::cout << "Sending RT INFO: "<< dr->getName() << " " <<dr->getId()<< std::endl;
					// }else{
					// 	std::cout << "DrWorker is -1" << std::endl;
					// }
				}else{
#ifdef DEBUG
					std::cout << "Not sending RT INFO: "<< dr->getName() << std::endl;
#endif
				}
			}
		}
	}

	// Send component for execution after sending data regions for global storage, if necessary
	MPI_Send(buff, comp_serialization_size, MPI_CHAR, worker_id, MessageTag::TAG_METADATA, this->comm_world);

#ifdef DEBUG
	std::cout << "Done send comp" << std::endl;
#endif
	delete[] buff;
}

void Manager::setWorkerSize(int worker_size)
{
    this->worker_size = worker_size;
}

void Manager::manager_process()
{

	uint64_t t1, t0;
#ifdef DEBUG
	std::cout<< "Manager ready. Rank = %d"<<std::endl;
#endif
	if(isFirstExecutionRound()){
		MPI_Barrier(this->comm_world);
		setFirstExecutionRound(false);
	}

	// now start the loop to listen for messages
	int curr = 0;
	int total = 10;
	MPI_Status status;
	int worker_id;
	char msg_type;
	int available_cpus;
	int available_gpus;
	int inputlen = 15;

	//TODO: testing only
	int tasksToFinishTasks = componentsToExecute->getSize();
#ifdef DEBUG
	std::cout << __FILE__ << ":" << __LINE__ << ". TasksToExecute="<<tasksToFinishTasks<<std::endl;
#endif

	// Process all components instantiated for execution
	while (componentsToExecute->getSize() != 0 || this->componentDependencies->getCountTasksPending() != 0 || this->getActiveComponentsSize()) {

		int flag;
		MPI_Iprobe(MPI_ANY_SOURCE, MessageTag::TAG_CONTROL, this->comm_world, &flag, &status);
		if (flag) {

			// Where is the message coming from
			worker_id=status.MPI_SOURCE;

			// Check the size of the input message
			int input_message_size;
			MPI_Get_count(&status, MPI_CHAR, &input_message_size);

			assert(input_message_size > 0);

			char *msg = new char[input_message_size];

			// Read the msg
			MPI_Recv(msg, input_message_size, MPI_CHAR, worker_id, MessageTag::TAG_CONTROL, this->comm_world, MPI_STATUS_IGNORE);
			msg_type = msg[0];
			available_cpus = msg[1];
			available_gpus = msg[2];

			switch(msg_type){
				case MessageTag::WORKER_READY:
				{
					if(this->componentsToExecute->getSize() > 0){
						PipelineComponentBase *compToExecute = NULL;

						// try to get a component pipeline that reuses data
						if(componentDataAwareSchedule){
							// try to get one of the prescheduled tasks for execution
							for(int i = 0; i < compToSchedDataAwareSchedule[worker_id].size(); i++){
								// get and erase first element from prescheduled list.
								int id = compToSchedDataAwareSchedule[worker_id].front();
								compToSchedDataAwareSchedule[worker_id].erase(compToSchedDataAwareSchedule[worker_id].begin());
								compToExecute = (PipelineComponentBase*)componentsToExecute->getByTaskId(id);
								if(compToExecute != NULL){
									break;
								}
							}
						}
						// if data reuse is not enabled or did not find a component to reuse data, try to get any.
						if(compToExecute == NULL){
							// select next component instantiation should be dispatched for execution
							if (this->halideQueue) {
								std::cout << "--------------------[Manager] getting task with avCPU=" 
									<< available_cpus << ", avGPU=" << available_gpus << std::endl;
								compToExecute = (PipelineComponentBase*)componentsToExecute->getTask(available_cpus, available_gpus);
							}
							else
								compToExecute = (PipelineComponentBase*)componentsToExecute->getTask();
							if (compToExecute == NULL) {
								// Tell worker that manager don't have a task for it. 
								// Nothing else to do at this moment. Should ask again.
								MPI_Send(&MessageTag::MANAGER_READY, 1, MPI_CHAR, worker_id, MessageTag::TAG_CONTROL, this->comm_world);
								break;
							}

							// compToExecute = (PipelineComponentBase*)componentsToExecute->getTask();
						}
						// tell worker that manager is ready
						MPI_Send(&MessageTag::MANAGER_READY, 1, MPI_CHAR, worker_id, MessageTag::TAG_CONTROL, this->comm_world);
#ifdef DEBUG
						std::cout << "Manager: before sending, size: "<< this->componentsToExecute->getSize() << std::endl;
#endif
						this->sendComponentInfoToWorker(worker_id, compToExecute);

						this->insertActiveComponent(compToExecute);

					}else{
						// tell worker that manager queue is empty. Nothing else to do at this moment. Should ask again.
						MPI_Send(&MessageTag::MANAGER_WORK_QUEUE_EMPTY, 1, MPI_CHAR, worker_id, MessageTag::TAG_CONTROL, this->comm_world);
					}
					break;
				}
				case MessageTag::WORKER_TASKS_COMPLETED:
				{
					std::vector<RTPipelineComponentBase*> components;

					// Pointer to the message area where the information about the tasks is stored
					int *tasks_data = (int*)(msg+sizeof(char));
					int number_components_completed = tasks_data[0];

					uint64_t t1 = Util::ClockGetTime();
#ifdef DEBUG
					std::cout << "Manager: #CompCompleted = "<< number_components_completed << " time: "<< t1<< " input_msg_size: "<< input_message_size<< " compId: "<< tasks_data[1] << std::endl;
#endif

					int extracted_size_bytes = sizeof(char) + sizeof(int);

					// calculate size of area in which components data is stored, its used to move to extra_data (RT) area
					for(int i = 0; i < number_components_completed; i++){
						// size of data of each component.
#ifdef DEBUG
						std::cout << "CompResultDataSize: " << ((int*)(msg+extracted_size_bytes))[1] << std::endl;
#endif

						extracted_size_bytes += 2 * sizeof(int) + ((int*)(msg+extracted_size_bytes))[1] * sizeof(char);
					}


					// components with extra data
					int number_components_extra_data = ((int*) (msg+extracted_size_bytes))[0]; //tasks_data[number_components_completed+1];
#ifdef DEBUG	
					std::cout << "Manager: #componets w/ extra data: "<< number_components_extra_data << " extracted bytes: "<< extracted_size_bytes<<std::endl;
#endif
					extracted_size_bytes += sizeof(int);

					//int extracted_size_bytes = sizeof(char) + sizeof(int) + number_components_completed * sizeof(int) + sizeof(int);

//#ifdef WITH_RT
					// This is only executed for RT components
					for(int i = 0; i < number_components_extra_data; i++){
						// size of the serialized data
						int size_component_data = 0;
						memcpy(&size_component_data, msg+extracted_size_bytes, sizeof(int));
//						std::cout << "\t Manager: size of component data: "<< size_component_data << std::endl;
						extracted_size_bytes += sizeof(int);

						RTPipelineComponentBase * rtComp = new RTPipelineComponentBase();
						int size_extracted_partial = rtComp->deserialize(msg+extracted_size_bytes);
#ifdef DEBUG
						if(size_component_data != size_extracted_partial){
							std::cout << "extracted is: "<<size_extracted_partial << " but sent is: "<< size_component_data<<std::endl;
						}
#endif
						extracted_size_bytes +=size_extracted_partial;
						components.push_back(rtComp);
					}

					// Do the magic and update the region templates
					// for each component received
					for(int i = 0; i < components.size(); i++){
						RTPipelineComponentBase *rtComp = components[i];
//						std::cout << "\t\tCOMP ID: "<< rtComp->getId() << " id in vector: "<< tasks_data[i+1]<< std::endl;

						// for each region template within this component
						for(int j = 0; j < rtComp->getNumRegionTemplates(); j++){
							RegionTemplate *rt = rtComp->getRegionTemplateInstance(j);

							// retrieve component with the same id as the received one
							RTPipelineComponentBase *rtCurr = dynamic_cast<RTPipelineComponentBase*>(this->getActiveComponent(rtComp->getId()));
							rtCurr->updateRegionTemplateInfo(rt);
							delete rt;
						}

						delete rtComp;
					}
					// END RT components only code section
//#endif

					extracted_size_bytes = sizeof(char) + sizeof(int);

					// Iterate over the component instances that completed and solve dependencies
					for(int i = 0; i < number_components_completed; i++){

#ifdef DEBUG
						//std::cout << "\t Solving dependencies for id="<< tasks_data[i+1] << std::endl;
						std::cout << "\t Solving dependencies for id="<< ((int*)(msg + extracted_size_bytes))[0] << std::endl;
#endif
						int compId = ((int*)(msg + extracted_size_bytes))[0];
						int resultDataSize = ((int*)(msg + extracted_size_bytes))[1];

						char *componentResultData = (char*)malloc(sizeof(char)*resultDataSize + sizeof(int));
						memcpy(componentResultData, msg+extracted_size_bytes+sizeof(int), sizeof(char)*resultDataSize + sizeof(int));

						this->insertComponentResultData(compId, componentResultData);

						extracted_size_bytes += 2 * sizeof(int) + resultDataSize * sizeof(char);

						// Retrieve component that just finished the execution, and delete it to resolve dependencies
						PipelineComponentBase * compPtrAux = this->retrieveActiveComponent(compId);

						// Assert that component was correctly found in the map of active components
						if (compPtrAux != NULL) {
							// if data aware schedule is used, find the dependent task that reuses
							// most of data and save its id to be used when Worker make a request
							if(componentDataAwareSchedule){
								std::list<Task *> dependentComponents = this->getDependentTasks(compPtrAux);
								std::list<Task *>::iterator depCompIt = dependentComponents.begin();

								long dataReuseSize = 0;
								int compIdReuse = -1;
#ifdef DEBUG
								std::cout << "WorkerId: "<<worker_id<<" "<<compPtrAux->getId() << " dependencies:";
#endif
								while(depCompIt != dependentComponents.end()){
									PipelineComponentBase *compReleased = ((PipelineComponentBase*)(*depCompIt));
									long compDataReuse = compReleased->getAmountOfDataReuse(worker_id);
									compDataReuse =1;

									// if this component reuses more data scheduling it to the worker_id
									if(compDataReuse > dataReuseSize  ){
										dataReuseSize = compDataReuse;
										compIdReuse = compReleased->getId();
									}
#ifdef DEBUG
									std::cout << " taskId: " << compReleased->getId() << " dataReused: "<< compReleased->getAmountOfDataReuse(worker_id);
#endif
									depCompIt++;
								}

								// if there is reuse, "preassign component for execution w/ this worker".
								// This is not a proper assignment, component w/ simply stay into a queue of preferred
								// execution w/ that given worker.
								if(dataReuseSize > 0 && compIdReuse != -1)
									this->compToSchedDataAwareSchedule[worker_id].push_back(compIdReuse);
#ifdef DEBUG
								std::cout << std::endl;
#endif
							}
							this->resolveDependencies(compPtrAux);

							delete compPtrAux;
						}else{
							std::cout << __FILE__ << ":"<< __LINE__ <<". Error: Component Id="<< compId << " not found!" <<std::endl;
						}
					}

					break;
				}
				default:
					std::cout << "Unknown message type="<< (int)msg_type <<std::endl;
					break;
			}

			delete[] msg;
		}else{
			usleep(1000);
		}

	}
	std::cout<< "Manager Leaving execution."<<std::endl;

}

std::list<Task *> Manager::getDependentTasks(PipelineComponentBase* pc) {
	return this->componentDependencies->retrieveDependentTasks(pc->getId());
}

bool Manager::isFirstExecutionRound() const {
	return FirstExecutionRound;
}

void Manager::setFirstExecutionRound(bool firstExecutionRound) {
	FirstExecutionRound = firstExecutionRound;
}

void Manager::insertComponentResultData(int id, char* data) {
	this->componentsResultData.insert(std::pair<int, char*>(id, data));
}

int Manager::insertComponentInstance(PipelineComponentBase* compInstance) {

//	compInstance->curExecEngine = NULL;
//	compInstance->managerContext = this;

	// Resolve component dependencies and queue it for execution, or left the component pending waiting
	this->componentDependencies->checkDependencies(compInstance, this->componentsToExecute);

	return 0;
}

int Manager::insertActiveComponent(PipelineComponentBase* pc) {
	int retValue = 0;
	assert(pc != NULL);

	this->activeComponents.insert(std::pair<int, PipelineComponentBase * >(pc->getId(), pc));
	return retValue;
}

PipelineComponentBase* Manager::retrieveActiveComponent(int id) {
	PipelineComponentBase* compRet = NULL;
	map<int, PipelineComponentBase*>::iterator activeCompIt;

	// Try to find component id that just finished
	activeCompIt = this->activeComponents.find(id);

	// If id is found, return component and removed it from the map.
	if(activeCompIt != this->activeComponents.end()){

		// Just assign to the return pointer the value of the component found
		compRet = activeCompIt->second;

		// Remove component instance from the map
		this->activeComponents.erase(activeCompIt);
#ifdef DEBUG
		std::cout << "Manager: components out processing = "<< this->activeComponents.size() << std::endl;
#endif
	}
	return compRet;
}

int Manager::getActiveComponentsSize() {
	return this->activeComponents.size();
}

int Manager::resolveDependencies(PipelineComponentBase* pc) {
	this->componentDependencies->resolveDependencies(pc, this->componentsToExecute);
	return 0;
}

PipelineComponentBase* Manager::getActiveComponent(int id) {
	PipelineComponentBase* compRet = NULL;
	map<int, PipelineComponentBase*>::iterator activeCompIt;

	// Try to find component id that just finished
	activeCompIt = this->activeComponents.find(id);

	// If id is found, return component and removed it from the map.
	if(activeCompIt != this->activeComponents.end()){

		// Just assign to the return point the value of the component found
		compRet = activeCompIt->second;
#ifdef DEBUG
		std::cout << "Manager: components out processing = "<< this->activeComponents.size() << std::endl;
#endif
	}
	return compRet;
}

char* Manager::getComponentResultData(int id) {
	char *retValue = NULL;
	std::map<int, char*>::iterator it = this->componentsResultData.find(id);
	if(it != this->componentsResultData.end()){
		retValue = it->second;
	}
	return retValue;
}

bool Manager::eraseResultData(int id) {
	bool retValue = false;
	std::map<int, char*>::iterator it = this->componentsResultData.find(id);
	if(it != this->componentsResultData.end()){
		retValue = true;
		free(it->second);
		this->componentsResultData.erase(it);
	}
		return retValue;
}
