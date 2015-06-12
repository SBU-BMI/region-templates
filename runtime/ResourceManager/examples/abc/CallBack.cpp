/*
 * CallBack.cpp
 *
 *  Created on: Feb 22, 2012
 *      Author: george
 */

#include "CallBack.h"

CallBack::CallBack() {
}

CallBack::~CallBack() {
}

bool CallBack::run(int procType, int tid)
{
	std::cout << "This is my call back."<<std::endl;
	return true;
}



