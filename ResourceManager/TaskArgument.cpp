/*
 * TaskArgument.cpp
 *
 *  Created on: Apr 2, 2012
 *      Author: gteodor
 */

#include "TaskArgument.h"

int TaskArgument::instancesIdCounter = 1;
pthread_mutex_t TaskArgument::taskCreationLock = PTHREAD_MUTEX_INITIALIZER;

TaskArgument::TaskArgument() {
	// Lock to guarantee that identifier counter is not read or updated in parallel
	pthread_mutex_lock(&TaskArgument::taskCreationLock);

	// Sets current identifier to the task
	this->setId(TaskArgument::instancesIdCounter);

	// Increments the unique id
	TaskArgument::instancesIdCounter++;

	// Most inefficient type, have to upload and download.
	this->setType(ExecEngineConstants::INPUT_OUTPUT);

	// Release lock
	pthread_mutex_unlock(&TaskArgument::taskCreationLock);
}

TaskArgument::~TaskArgument() {

}

