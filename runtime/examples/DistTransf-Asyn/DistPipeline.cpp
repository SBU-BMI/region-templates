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
#include "DistComp.h"
#include "ReadInputFileNames.h"


int main (int argc, char **argv){
	SysEnv sysEnv;

	if(argc < 4){
		std::cout << "Usage:  " << argv[0] << " <marker> <tileSize> " << std::endl;
		return -1;
	}
	// Initialize input dir/image and outDir from user parameters
	std::string imageName = argv[1];
	std::string tileSize = argv[2];

	// Startup the computation environment
	sysEnv.startupSystem(argc, argv, "libdistasyncomponents.so");

	// Set component arguments
	DistComp *cp = new DistComp();
	cp->addArgument(new ArgumentString(imageName));
	cp->addArgument(new ArgumentString(tileSize));

	sysEnv.executeComponent(cp);

	// End Create Dependency Graph
	sysEnv.startupExecution();

	// Finalize all processes running and end execution
	sysEnv.finalizeSystem();

	return 0;
}



