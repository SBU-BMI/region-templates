/*
 * DataRegionFactory.h
 *
 *  Created on: Feb 15, 2013
 *      Author: george
 */

#ifndef DATAREGIONFACTORY_H_
#define DATAREGIONFACTORY_H_

#include "DataRegion.h"
#include "DenseDataRegion2D.h"

// OpenCV library includes: used as an auxiliar lib to read/write data(images) to fs
//#include "cv.hpp"
//#include "opencv2/gpu/gpu.hpp"
#include "highgui.h"
#include <string>
#include <sstream>

#ifdef WITH_DATA_SPACES

extern "C" {
#include "dataspaces.h"
}

#endif
class DenseDataRegion2D;

class DataRegionFactory {
private:
	static bool readDDR2DFS(DenseDataRegion2D *dataRegion, int chunkId=-1, std::string path="");
	static bool writeDDR2DFS(DenseDataRegion2D *dataRegion, std::string path="");

	static bool readDDR2DATASPACES(DenseDataRegion2D* dataRegion);
	static bool writeDDR2DATASPACES(DenseDataRegion2D* dataRegion);

	friend class CacheComponent;
	friend class Cache;
public:
	DataRegionFactory();
	virtual ~DataRegionFactory();

	static bool instantiateDataRegion(DataRegion *dr, int chunkId=-1, std::string path="");
	static bool stageDataRegion(DataRegion *dr);
};

#endif /* DATAREGIONFACTORY_H_ */
