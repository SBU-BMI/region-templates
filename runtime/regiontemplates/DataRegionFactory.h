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
#include "DataRegion2DUnaligned.h"

// OpenCV library includes: used as an auxiliar lib to read/write data(images) to fs
//#include "cv.hpp"
//#include "opencv2/gpu/gpu.hpp"
// #include "highgui.h" // old opencv 2.4
#ifdef WITH_CUDA
#include "opencv2/cudaarithm.hpp" // new opencv 3.4.1
#endif

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

	static bool readDDR2DATASPACES(DenseDataRegion2D* dataRegion);
		static bool writeDDR2DATASPACES(DenseDataRegion2D* dataRegion);

	static bool createLockFile(DataRegion* dr, std::string outputFile);
	static int lockFileExists(DataRegion* dr, std::string path);
	static std::string createOutputFileName(DataRegion* dr, std::string path, std::string extension);

	static bool writeDr2DUn(DataRegion2DUnaligned* dr, std::string outputFile);
	static bool readDr2DUn(DataRegion2DUnaligned* dr, std::string inputFile);

	friend class CacheComponent;
	friend class Cache;
public:
	static bool readDDR2DFS(DataRegion **dataRegion, int chunkId=-1, std::string path="", bool ssd=false);
	static bool writeDDR2DFS(DataRegion *dataRegion, std::string path="", bool ssd=false);


	DataRegionFactory();
	virtual ~DataRegionFactory();

	/*static bool instantiateDataRegion(DataRegion *dr, int chunkId=-1, std::string path="");
	static bool stageDataRegion(DataRegion *dr);*/
};

#endif /* DATAREGIONFACTORY_H_ */
