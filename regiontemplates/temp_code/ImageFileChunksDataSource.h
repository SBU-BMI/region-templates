/*
 * ImageFileChunksDataSource.h
 *
 *  Created on: Oct 24, 2012
 *      Author: george
 */

#ifndef IMAGEFILECHUNKSDATASOURCE_H_
#define IMAGEFILECHUNKSDATASOURCE_H_

#include "BasicDataSource.h"
#include "BoundingBox.h"
#include "FileUtils.h"
#include "DenseDataRegion2D.h"
#include "Constants.h"
// OpenCV library includes
//#include "cv.hpp"
#include "highgui.h"

class ImageFileChunksDataSource: public BasicDataSource {
private:
	std::vector<std::pair<BoundingBox,std::string> > dataFilePaths;
	ImageFileChunksDataSource();
	int tileSize;
	vector<string> tokenize(const string & str, const string & delim);
	std::string folderName;
	std::string extension;
	std::string outFolder;

public:
	ImageFileChunksDataSource(std::string folderPath, std::string extension, int tileSize, std::string outFolder = "");
	virtual ~ImageFileChunksDataSource();
	bool instantiateRegion(DataRegion *regionData);
	bool writeRegion(DataRegion *dataRegion);
	void print();
	int getTileSize() const;
	void setTileSize(int tileSize);
};

#endif /* IMAGEFILECHUNKSDATASOURCE_H_ */
