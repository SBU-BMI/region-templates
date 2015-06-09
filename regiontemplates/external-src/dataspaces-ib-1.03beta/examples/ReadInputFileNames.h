/*
 * ReadInputFileNames.h
 *
 *  Created on: Mar 2, 2012
 *      Author: george
 */

#ifndef READINPUTFILENAMES_H_
#define READINPUTFILENAMES_H_

#include "FileUtils.h"
#include "string.h"

class ReadInputFileNames {
public:
	ReadInputFileNames();
	virtual ~ReadInputFileNames();

	static void getFiles(const std::string &imageName, const std::string &outDir, std::vector<std::string> &filenames, std::vector<std::string> &seg_output);

};

#endif /* READINPUTFILENAMES_H_ */
