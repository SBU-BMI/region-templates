/*
 * ReadInputFileNames.h
 *
 *  Created on: Mar 2, 2012
 *      Author: george
 */

#ifndef READINPUTFILENAMES_H_
#define READINPUTFILENAMES_H_

#include <iostream>
#include "FileUtils.h"
#include "string.h"
#include <stdlib.h>
#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"

class ReadInputFileNames {
public:
	ReadInputFileNames();
	virtual ~ReadInputFileNames();

	static void getFiles(const std::string &imageName, const std::string &outDir, std::vector<std::string> &filenames, std::vector<std::string> &seg_output);
	static void getDimensions(int &x, int &y, std::vector<std::string> &fileNames);
	static void assembleTiles(cv::Mat &image, std::vector<std::string> &fileNames);
};

#endif /* READINPUTFILENAMES_H_ */
