/*
 * HelloWorld.cpp
 *
 *  Created on: Feb 15, 2012
 *      Author: george
 */

#include "SysEnv.h"
#include <sstream>
#include <stdlib.h>
#include <iostream>
#include "PipelineComponent.h"
#include "FileUtils.h"


int main (int argc, char **argv){
	SysEnv sysEnv;

	sysEnv.startupSystem(argc, argv, "libcomponentsPipeTasks.so");
	// Create Dependency Graph

	// first get the list of files to process
   	std::vector<std::string> filenames;
	
	FileUtils fUtils(".tiff");
	fUtils.traverseDirectory(argv[1], filenames, FileUtils::FILE, true);

	std::cout << "#files: "<< filenames.size() <<std::endl; 

	for(int i = 0; i < filenames.size(); i++){

		PipelineComponent *pc = new PipelineComponent();
		pc->addArgument(new ArgumentString(filenames[i]));

		sysEnv.executeComponent(pc);
	}
	// End Create Dependency Graph
	sysEnv.startupExecution();

	// Finalize all processes running and end execution
	sysEnv.finalizeSystem();

	return 0;
}



