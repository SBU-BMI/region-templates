/*
 * PipelineComponentBase.cpp
 *
 *  Created on: Feb 16, 2012
 *      Author: george
 */

#include "PipelineComponentBase.h"

int PipelineComponentBase::instancesIdCounter = 1;

PipelineComponentBase::PipelineComponentBase(){
	// Sets current identifier to the task
	this->setId(PipelineComponentBase::instancesIdCounter);

	// Increments the unique id
	PipelineComponentBase::instancesIdCounter++;
//	this->setManagerContext(NULL);
	this->setType(PipelineComponentBase::PIPELINE_COMPONENT_BASE);
	this->setLocation(PipelineComponentBase::MANAGER_SIDE);
}

PipelineComponentBase::~PipelineComponentBase() {
/*	if(this->getManagerContext() != NULL){
//		std::cout << __FILE__ << ":" << __LINE__ << ". Resolving dependencies of compId="<< this->getId() << std::endl;
//		std::cout << "CompSizeBefore=" << this->getManagerContext()->componentsToExecute->getSize() << std::endl;
//		this->getManagerContext()->resolveDependencies(this);
//		std::cout << "CompSizeAfter=" << this->getManagerContext()->componentsToExecute->getSize() << std::endl;

	}*/
	while(this->arguments.size()> 0){
		ArgumentBase *aux = this->arguments.back();
		this->arguments.pop_back();
		delete aux;
	}
}

void PipelineComponentBase::addArgument(ArgumentBase *arg)
{
	this->arguments.push_back(arg);
}



ArgumentBase *PipelineComponentBase::getArgument(int index)
{
	ArgumentBase *retArg = NULL;
	if(index >= 0 && index < this->arguments.size()){
		retArg = this->arguments[index];
	}
	return retArg;
}



std::string PipelineComponentBase::getComponentName() const
{
    return component_name;
}

void PipelineComponentBase::setComponentName(std::string component_name)
{
    this->component_name = component_name;
}

int PipelineComponentBase::getArgumentsSize()
{
	return (int)this->arguments.size();
}

// Stuff related to the component factory
std::map<std::string, componetFactory_t* > PipelineComponentBase::ComponentFactory::factoryMap;

bool PipelineComponentBase::ComponentFactory::componentRegister(std::string name, componetFactory_t *filterFactory)
{
	factoryMap.insert(std::pair<std::string, componetFactory_t*>(name, filterFactory) );
	return true;
}



componetFactory_t *PipelineComponentBase::ComponentFactory::getComponentFactory(std::string name)
{
	componetFactory_t* factoryRet = NULL;
	std::map<std::string, componetFactory_t*>::iterator map_it;
	map_it = factoryMap.find(name);

	if(map_it!=factoryMap.end()){
		factoryRet = map_it->second;
	}
//	std::cout << "factoryMap.size()="<< factoryMap.size() <<std::endl;
	return factoryRet;

}

PipelineComponentBase *PipelineComponentBase::ComponentFactory::getCompoentFromName(std::string name)
{
	PipelineComponentBase* pc = NULL;
	componetFactory_t* factoryFunction = PipelineComponentBase::ComponentFactory::getComponentFactory(name);
	if(factoryFunction != NULL){
		pc = factoryFunction();
	}
	return pc;
}

int PipelineComponentBase::size()
{
	// To store component id
	int size_bytes = sizeof(int);

	// To store the size of the name
	size_bytes += sizeof(int);

	// The proper name of the component class
	size_bytes += this->getComponentName().size() * sizeof(char);

	// To store the quantity of arguments
	size_bytes += sizeof(int);

	// the size of each argument
	for(int i = 0; i < this->getArgumentsSize(); i++){
		size_bytes+=this->getArgument(i)->size();
	}
	return size_bytes;
}

int PipelineComponentBase::serialize(char *buff)
{
	int serialized_bytes = 0;

	// Copy component id to buffer
	((int*)buff)[0] = this->getId();
	serialized_bytes += sizeof(int);

	// Get size of the component name
	int name_size = this->getComponentName().size();

	// Copy size of component name name to buffer
	memcpy(buff+serialized_bytes, &name_size, sizeof(int));

	// increase number of bytes written to buffer
	serialized_bytes+=sizeof(int);

	// Copy the actual name of the component to the buffer
	memcpy(buff+serialized_bytes, this->getComponentName().c_str(), name_size*sizeof(char) );
	serialized_bytes+=name_size*sizeof(char);

	// Copy the number of arguments
	int number_args = this->getArgumentsSize();
	memcpy(buff+serialized_bytes, &number_args, sizeof(int));
	serialized_bytes+=sizeof(int);

	// serialize each of the arguments
	for(int i = 0; i < number_args; i++){
		serialized_bytes += this->getArgument(i)->serialize(buff+serialized_bytes);
	}
//	std::cout << "PipelineComponentBase::serialize" << std::endl;
	return serialized_bytes;
}



int PipelineComponentBase::getId() const
{
    return id;
}


ExecutionEngine *PipelineComponentBase::getResourceManager() const
{
    return resourceManager;
}

void PipelineComponentBase::setResourceManager(ExecutionEngine *resourceManager)
{
    this->resourceManager = resourceManager;
}

void PipelineComponentBase::setId(int id)
{
    this->id = id;
}

int PipelineComponentBase::deserialize(char *buff)
{
	int deserialized_bytes = 0;
	this->setId(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// Extract the size of the component name
	int name_size = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes+=sizeof(int);

	// Copy the name of the component to aux variable
	char component_name[name_size+1];
	component_name[name_size] = '\0';
	memcpy(component_name, buff+deserialized_bytes, sizeof(char)*name_size);
	deserialized_bytes += sizeof(char)*name_size;

	// set the component name
	this->setComponentName(component_name);

	// Deserialize the number of arguments used by component
	int number_args = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes+=sizeof(int);

	// deserialize each of the arguments
	for(int i = 0; i < number_args; i++){
		// get the type of the argument. Warning, do not advance the buffer pointer,
		// because the type is extracted again during argument deserialization.
		int arg_type = ((int*)(buff+deserialized_bytes))[0];

		ArgumentBase *arg = new ArgumentString();
		// finally deserialize the argument, and move the buffer pointer
		deserialized_bytes += arg->deserialize(buff+deserialized_bytes);

		this->addArgument(arg);

	}
//	std::cout << "PipelineComponentBase::deserialize" << std::endl;
	return deserialized_bytes;

}

void PipelineComponentBase::executeTask(Task *task)
{
	if(this->getResourceManager() != NULL){
		this->getResourceManager()->insertTask(task);
	}else{
		std::cout << __FILE__ << ":" << __LINE__ << ". Could not dispatch task for execution. Resource manager is NULL!"<<std::endl;
	}
}

int PipelineComponentBase::getType() const {
	return type;
}

int PipelineComponentBase::getLocation() const {
	return location;
}

void PipelineComponentBase::setLocation(int location) {
	this->location = location;
}

void PipelineComponentBase::setType(int type) {
	this->type = type;
}

long PipelineComponentBase::getAmountOfDataReuse(int workerId) {
	return 0;
}
