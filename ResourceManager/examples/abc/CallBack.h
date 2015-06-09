/*
 * CallBack.h
 *
 *  Created on: Feb 22, 2012
 *      Author: george
 */

#ifndef CALLBACK_H_
#define CALLBACK_H_

#include "Task.h"

class CallBack: public CallBackTaskBase {
public:
	CallBack();
	virtual ~CallBack();
	bool run(int procType=ExecEngineConstants::GPU, int tid=0);
};

#endif /* CALLBACK_H_ */
