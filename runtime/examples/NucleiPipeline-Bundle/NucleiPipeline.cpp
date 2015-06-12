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
#include "CompSeg.h"
#include "ReadInputFileNames.h"


int main (int argc, char **argv){
	SysEnv sysEnv;

	// first get the list of files to process
   	std::vector<std::string> filenames;
	std::vector<std::string> seg_output;

	if(argc < 3){
		std::cout << "Usage:  " << argv[0] << " <image_filename | image_dir> outdir " << std::endl;
		return -1;
	}
	// Initialize input dir/image and outDir from user parameters
	std::string imageName = argv[1], outDir = argv[2];

	// Startup the computation environment
	sysEnv.startupSystem(argc, argv, "libnpipecomponentsbundle.so");

	ReadInputFileNames::getFiles(imageName, outDir, filenames, seg_output);

	std::cout << "#Files: "<< filenames.size() << std::endl;
// Loop used in weakscaling execution
//	for(int j = 0; j < sysEnv.getWorkerSize(); j++){
		// Instantiate one replica of the pipeline for each input element
		for(int i = 0; i < filenames.size(); i++){
			CompSeg *cp = new CompSeg();
			cp->addArgument(new ArgumentString(filenames[i]));
			cp->addArgument(new ArgumentString(seg_output[i]));

			sysEnv.executeComponent(cp);

		}
//	}
	// End Create Dependency Graph
	sysEnv.startupExecution();

	// Finalize all processes running and end execution
	sysEnv.finalizeSystem();

	return 0;
}



