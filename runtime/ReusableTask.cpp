/*
 * ReusableTask.cpp
 *
 *  Created on: Aug 1, 2016
 *      Author: willian
 */

#include "ReusableTask.hpp"

// Stuff related to the task factory
std::map<std::string, task_factory_t1* > ReusableTask::ReusableTaskFactory::factoryMap1;
std::map<std::string, task_factory_t2* > ReusableTask::ReusableTaskFactory::factoryMap2;

bool ReusableTask::ReusableTaskFactory::taskRegister(std::string name, task_factory_t1 *filterFactory1, task_factory_t2 *filterFactory2)
{
	factoryMap1.insert(std::pair<std::string, task_factory_t1*>(name, filterFactory1) );
	factoryMap2.insert(std::pair<std::string, task_factory_t2*>(name, filterFactory2) );
	return true;
}

task_factory_t1 *ReusableTask::ReusableTaskFactory::getTaskFactory1(std::string name)
{
	task_factory_t1* factoryRet = NULL;
	std::map<std::string, task_factory_t1*>::iterator map_it;
	map_it = factoryMap1.find(name);

	if(map_it!=factoryMap1.end()){
		factoryRet = map_it->second;
	}
//	std::cout << "factoryMap1.size()="<< factoryMap1.size() <<std::endl;
	return factoryRet;

}

ReusableTask *ReusableTask::ReusableTaskFactory::getTaskFromName(std::string name, list<ArgumentBase*> args, RegionTemplate* inputRt)
{
	ReusableTask* t = NULL;
	task_factory_t1* factoryFunction = ReusableTask::ReusableTaskFactory::getTaskFactory1(name);
	if(factoryFunction != NULL){
		t = factoryFunction(args, inputRt);
	}
	return t;
}

task_factory_t2 *ReusableTask::ReusableTaskFactory::getTaskFactory2(std::string name)
{
	task_factory_t2* factoryRet = NULL;
	std::map<std::string, task_factory_t2*>::iterator map_it;
	map_it = factoryMap2.find(name);

	if(map_it!=factoryMap2.end()){
		factoryRet = map_it->second;
	}
//	std::cout << "factoryMap2.size()="<< factoryMap2.size() <<std::endl;
	return factoryRet;

}

ReusableTask *ReusableTask::ReusableTaskFactory::getTaskFromName(std::string name)
{
	ReusableTask* t = NULL;
	task_factory_t2* factoryFunction = ReusableTask::ReusableTaskFactory::getTaskFactory2(name);
	if(factoryFunction != NULL){
		t = factoryFunction();
	}
	return t;
}