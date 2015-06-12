/*
 * ImageFileChunksDataSource.h
 *
 *  Created on: Oct 24, 2012
 *      Author: george
 */

#ifndef FILE_SYSTEM_IMAGE_DATA_SOURCE_H_
#define FILE_SYSTEM_IMAGE_DATA_SOURCE_H_

#include "DenseDataRegion2D.h"
#include "BasicDataSource.h"
#include "BoundingBox.h"
#include "FileUtils.h"

#include "Constants.h"
// OpenCV library includes
//#include "cv.hpp"
#include "highgui.h"

class BasicDataSource;

class FileSystemImageDataSource: public BasicDataSource {
private:
	FileSystemImageDataSource();

public:
	FileSystemImageDataSource(std::string fileName);
	virtual ~FileSystemImageDataSource();
	bool instantiateRegion(DataRegion *regionData);
	bool writeRegion(DataRegion *dataRegion);
	void print();
};

#endif /* FILE_SYSTEM_IMAGE_DATA_SOURCE_H_ */
