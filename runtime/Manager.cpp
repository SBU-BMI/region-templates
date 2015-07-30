/*
 * Manager.cpp
 *
 *  Created on: Feb 15, 2012
 *      Author: george
 */

#include "Manager.h"

Manager::Manager(const MPI::Intracomm& comm_world, const int manager_rank, const int worker_size, const bool componentDataAwareSchedule, const int queueType) {
	this->comm_world =comm_world;
	this->manager_rank = manager_rank;
	this->worker_size = worker_size;
	setFirstExecutionRound(true);
	this->componentDataAwareSchedule = componentDataAwareSchedule;
	for(int i = 0; i < worker_size; i++){
		std::vector<int> aux;
		this->compToSchedDataAwareSchedule.push_back(aux);
	}
	std::cout << "compToSchedSize: " << this->compToSchedDataAwareSchedule.size() << std::endl;

	if(queueType ==ExecEngineConstants::FCFS_QUEUE){
		componentsToExecute = new TasksQueueFCFS(1, 0);
	}else{
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

MPI::Intracomm Manager::getCommWorld() const
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
	MPI::Status status;
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

		if (comm_world.Iprobe(MPI_ANY_SOURCE, MessageTag::TAG_CONTROL, status)) {

			// where is it coming from
			worker_id=status.Get_source();
			comm_world.Recv(&ready, 1, MPI::CHAR, worker_id, MessageTag::TAG_CONTROL);

			if (worker_id == manager_rank) continue;

			if(ready == MessageTag::WORKER_READY) {
				if(finished[worker_id] == false){
					comm_world.Send(&MessageTag::MANAGER_FINISHED, 1, MPI::CHAR, worker_id, MessageTag::TAG_CONTROL);
					printf("manager signal finished worker: %d manager_rank: %d\n", worker_id, manager_rank);
					--active_workers;
					finished[worker_id] = true;
				}
			}
		}
	}
	this->getCommWorld().Barrier();
	MPI::Finalize();
	return 0;
}

void Manager::checkConfiguration()
{

	MPI::Status status;
	int worker_id;
	char ready;
	bool correctInitialization = true;

	int active_workers = worker_size;

	// Listening from each worker whether it was initialized correctly
	while (active_workers > 0) {
		usleep(1000);

		if (comm_world.Iprobe(MPI_ANY_SOURCE, MessageTag::TAG_CONTROL, status)) {

			// where is it coming from
			worker_id=status.Get_source();
			bool curWorkerStatus;
			comm_world.Recv(&curWorkerStatus, 1, MPI::BOOL, worker_id, MessageTag::TAG_CONTROL);
			if(curWorkerStatus == false){
				correctInitialization = false;
			}
			active_workers--;
		}
	}

	// Tell each worker whether to continue or quit the execution
	comm_world.Bcast(&correctInitialization, 1 , MPI::BOOL, this->getManagerRank());

	if(correctInitialization==false){
		std::cout << "Quitting. Workers initialization failed. Possible due to errors loading the components library."<<std::endl;
		MPI::Finalize();
		exit(1);
	}
}


// send message to Worker requesting for a data region to be send to global storage
void Manager::sendDRInfoToWorkerForGlobalStorage(int worker_id, std::string rtName, std::string rtId,
		DataRegion* dr) {
	// tell worker that a DR should be moved to global storage
	comm_world.Send(&MessageTag::CACHE_MOVE_DR_TO_GLOBAL_STORAGE, 1, MPI::CHAR, worker_id, MessageTag::TAG_CONTROL);


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


	comm_world.Send(msg, msgSize, MPI::CHAR, worker_id, MessageTag::TAG_CACHE_INFO);

	delete msg;
}

void Manager::sendComponentInfoToWorker(int worker_id, PipelineComponentBase *pc)
{
	std::cout << "Manager: Sending component id="<< pc->getId() <<std::endl;
	int comp_serialization_size = pc->size();
	char *buff = new char[comp_serialization_size];
	int used_serialization_size = pc->serialize(buff);
	assert(comp_serialization_size == used_serialization_size);

	comm_world.Send(buff, comp_serialization_size, MPI::CHAR, worker_id, MessageTag::TAG_METADATA);

	if(pc->getType() == PipelineComponentBase::RT_COMPONENT_BASE){
		//std::cout << "This is a PipelineComponentBase::RT_COMPONENT_BASE" << std::endl;
		RTPipelineComponentBase *rtCurr = dynamic_cast<RTPipelineComponentBase*>(pc);
		// check whether there is any data region that is local but should be sent to a global storage
		// for each region template within this component
		for(int j = 0; j < rtCurr->getNumRegionTemplates(); j++){
			RegionTemplate *rt = rtCurr->getRegionTemplateInstance(j);
			for(int i = 0; i < rt->getNumDataRegions(); i++){
				DataRegion *dr = rt->getDataRegion(i);
				// if data region is not an application input, is not on global storage,
				// or local with the work to which this component was assigned
				/*if(!dr->getIsAppInput() && dr->getCacheType() != Cache::GLOBAL && dr->getWorkerId() != worker_id){
					if(dr->getWorkerId() != -1){
						this->sendDRInfoToWorkerForGlobalStorage(dr->getWorkerId(), rt->getName(), rt->getId(), dr);
						std::cout << "Sending RT INFO: "<< dr->getName() << " " <<dr->getId()<< std::endl;
					}else{
						std::cout << "DrWorker is -1" << std::endl;
					}
				}else{
					std::cout << "Not sending RT INFO: "<< dr->getName() << std::endl;
				}*/
			}
		}
	}
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

	std::cout<< "Manager ready. Rank = %d"<<std::endl;
	if(isFirstExecutionRound()){
		comm_world.Barrier();
		setFirstExecutionRound(false);
	}

	// now start the loop to listen for messages
	int curr = 0;
	int total = 10;
	MPI::Status status;
	int worker_id;
	char msg_type;
	int inputlen = 15;

	//TODO: testing only
	int tasksToFinishTasks = componentsToExecute->getSize();
	std::cout << __FILE__ << ":" << __LINE__ << ". TasksToExecute="<<tasksToFinishTasks<<std::endl;

	// Process all components instantiated for execution
	while (componentsToExecute->getSize() != 0 || this->componentDependencies->getCountTasksPending() != 0 || this->getActiveComponentsSize()) {

		if (comm_world.Iprobe(MPI_ANY_SOURCE, MessageTag::TAG_CONTROL, status)) {

			// Where is the message coming from
			worker_id=status.Get_source();

			// Check the size of the input message
			int input_message_size = status.Get_count(MPI::CHAR);

			assert(input_message_size > 0);

			char *msg = new char[input_message_size];

			// Read the
			comm_world.Recv(msg, input_message_size, MPI::CHAR, worker_id, MessageTag::TAG_CONTROL);
			//			printf("manager received request from worker %d\n",worker_id);
			msg_type = msg[0];

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
							compToExecute = (PipelineComponentBase*)componentsToExecute->getTask();
						}
						// tell worker that manager is ready
						comm_world.Send(&MessageTag::MANAGER_READY, 1, MPI::CHAR, worker_id, MessageTag::TAG_CONTROL);

						std::cout << "Manager: before sending, size: "<< this->componentsToExecute->getSize() << std::endl;
						this->sendComponentInfoToWorker(worker_id, compToExecute);

						this->insertActiveComponent(compToExecute);

					}else{
						// tell worker that manager queue is empty. Nothing else to do at this moment. Should ask again.
						comm_world.Send(&MessageTag::MANAGER_WORK_QUEUE_EMPTY, 1, MPI::CHAR, worker_id, MessageTag::TAG_CONTROL);
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

					std::cout << "Manager: #CompCompleted = "<< number_components_completed << " time: "<< t1<< " input_msg_size: "<< input_message_size<< " compId: "<< tasks_data[1] << std::endl;

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

		std::cout << "Manager: components out processing = "<< this->activeComponents.size() << std::endl;
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

		std::cout << "Manager: components out processing = "<< this->activeComponents.size() << std::endl;
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
