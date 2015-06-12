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
#include "MophReconComp.h"
#include "ReadInputFileNames.h"


int main (int argc, char **argv){
	SysEnv sysEnv;

	// first get the list of files to process
   	std::vector<std::string> filenames;
	std::vector<std::string> seg_output;

	if(argc < 4){
		std::cout << "Usage:  " << argv[0] << " <marker> <mask> <tileSize> " << std::endl;
		return -1;
	}
	// Initialize input dir/image and outDir from user parameters
	std::string imageName = argv[1];
	std::string maskName = argv[2];
	std::string tileSize = argv[3];

	// Startup the computation environment
	sysEnv.startupSystem(argc, argv, "libreconcomponents.so");

//	ReadInputFileNames::getFiles(imageName, outDir, filenames, seg_output);

//	for(int i = 0; i < filenames.size(); i++){
		MophReconComp *cp = new MophReconComp();
		cp->addArgument(new ArgumentString(imageName));
		cp->addArgument(new ArgumentString(maskName));
		cp->addArgument(new ArgumentString(tileSize));

		sysEnv.executeComponent(cp);

//	}
	// End Create Dependency Graph
	sysEnv.startupExecution();

	// Finalize all processes running and end execution
	sysEnv.finalizeSystem();

	return 0;
}



