/*
 * MessageTag.h
 *
 *  Created on: Feb 15, 2012
 *      Author: george
 */

#ifndef MESSAGETAG_H_
#define MESSAGETAG_H_

namespace MessageTag {

	// Communication Tags. Used to filter messages and corresponds to MPI tags
	static const int TAG_CONTROL = 0;
	static const int TAG_DATA = 1;
	static const int TAG_METADATA = 2;
	static const int TAG_CONFIGURATION = 4;
	static const int TAG_CACHE_INFO = 5;

	// Type of message
	// Manager is ready to sent another chunk of work
	static const char MANAGER_READY = 10;

	// Manager is finished, not more work will be sent
	static const char MANAGER_FINISHED = 11;

	// Error in the execution. Abort it
	static const char MANAGER_ERROR = 12;

	// Manager does not have any work available for assignment
	static const char MANAGER_WORK_QUEUE_EMPTY = 13;

	// Worker is requesting another task
	static const char WORKER_READY = 20;

	// Sent component instances completed information to the Manager
	static const char WORKER_TASKS_COMPLETED = 21;

	// Message from Manager to Worker, informing that a given cached
	// data region should be sent to a global storage
	static const char CACHE_MOVE_DR_TO_GLOBAL_STORAGE = 22;

};

#endif /* MESSAGETAG_H_ */
