/*
 * HelloWorld.cpp
 *
 *  Created on: Feb 15, 2012
 *      Author: george
 */

#include <sstream>
#include <stdlib.h>
#include <iostream>
#include "SysEnv.h"
#include "CompA.h"
#include "CompB.h"

#define NUM_PIPELINE_INSTANCES	10

int main (int argc, char **argv){
	SysEnv sysEnv;

	sysEnv.startupSystem(argc, argv, "libcomponents.so");
	// Create Dependency Graph

	CompA *cpCache;

	for(int i = 0; i < NUM_PIPELINE_INSTANCES; i++){
		std::stringstream ss;//create a stringstream
		ss << i;//add number to the stream

		CompA *cp = new CompA();
		cp->addArgument(new ArgumentString(ss.str()));

		if(i>0)
			cp->addDependency(cp->getId()-2);

		// Instantiate second stage of the pipeline, which depends on the first
		CompB *cpB = new CompB();
		cpB->addArgument(new ArgumentString(ss.str()));
		cpB->addDependency(cp->getId());

		sysEnv.executeComponent(cp);
		sysEnv.executeComponent(cpB);
	}
	// End Create Dependency Graph
	sysEnv.startupExecution();

	// Finalize all processes running and end execution
	sysEnv.finalizeSystem();

	return 0;
}



