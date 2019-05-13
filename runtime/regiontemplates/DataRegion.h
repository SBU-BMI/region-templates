/*
 * DataRegion.h
 *
 *  Created on: Oct 22, 2012
 *      Author: george
 */

#ifndef DATA_H_
#define DATA_H_

#include <string>
#include <iostream>
#include "Constants.h"
#include "BoundingBox.h"

//#include "BasicDataSource.h"
//#include "FileSystemImageDataSource.h"


class BasicDataSource;
//class FileSystemImageDataSource;

class DataRegion {
private:
	int elementsType; // Type of each data element within the region: CHAR, UCHAR, INT, UINT, etc.
	int regionType;	   // Dense, sparse, (2D/3D)...
	int version;      // User specified version for the region
	int timestamp;    // Time stamp to which region is associated
	std::string name; // name identifier
	int resolution;   // resolution of the region
	BoundingBox bb;   // bounding box surrounding the region
	std::string id;   // this id identifies a given instance of the data region and
					  // is used to read/write the data region
	int storageLevel; // if set, the data will be written directly to a certain storage level.
					  // Otherwise, its written in higher level in which it fits.

	// this is used if the data region is created by the user from files. In this case,
	// I'll used the file name provided by the uses, whereas in data regions created
	// during the execution the system creates unique id and manage them automatically
	bool isAppInput;	//
	std::string inputFileName; //  complete path

	BoundingBox ROI;  // region of interest inside a given data region

	// This structure associates a given bounding box to a id which identifies the
	// source data storage for that bounding box. It is used for cases in which
	// the data region is divided into different pieces, i.e. multiple files
	std::vector<std::pair<BoundingBox, std::string> > bb2Id; // it id used to retrieve element from storage system (filesystem, ADIOS, DataSpaces, etc)

	// Abstraction responsible for instantiating the region template. The
	// actual data may be stored in a distributed shared memory or file chunks
	// and it will be instantiated on demand using this object.
	int inputType;
	int outputType;
	int outputExtension;

	bool isSvs;

	friend class Cache;
	friend class CacheComponent;
	friend class RTPipelineComponentBase;
	friend class Manager;
	friend class RegionTemplate;

protected:
	// cache level, type, and id of the worker in which it is stored
	int cacheLevel;
	int cacheType;
	int workerId;
	long cachedDataSize;

	// use to know if a given data region was changed by a component, thus to know if it has to be sent to storage
	bool modified;

	// ROI bounding box for svs files
	cv::Rect_<int64_t> roi;

	int getCacheLevel() const;
	void setCacheLevel(int cacheLevel);
	int getCacheType() const;
	void setCacheType(int cacheType);
	int getWorkerId() const;
	void setWorkerId(int workerId);
	bool isModified() const;
	void setModified(bool modified);

public:
	// output storage extensions
	static const int PBM = 1;
	static const int XML = 2;

	DataRegion();
	virtual ~DataRegion();
	int getType() const;
	void setType(int type);

	const BoundingBox& getBb() const;
	void setBb(const BoundingBox& bb);

	void setName(std::string name);
	int getResolution() const;
	void setResolution(int resolution);
	int getTimestamp() const;
	void setTimestamp(int timestamp);
	int getVersion() const;
	void setVersion(int version);
	std::string getName() const;
	virtual bool instantiateRegion();
	virtual bool write();
	virtual bool empty();
	virtual DataRegion* clone(bool copyData);
	virtual long getDataSize();

	void insertBB2IdElement(BoundingBox bb, std::string id);
	std::pair<BoundingBox, std::string> getBB2IdElement(int index);
	int getBB2IdSize();

	int serialize(char* buff);
	int deserialize(char* buff);
	int serializationSize();

	int getInputType() const;
	void setInputType(int inputType);
	int getOutputType() const;
	void setOutputType(int outputType);
	std::string getId() const;
	void setId(std::string id);
	const BoundingBox& getROI() const;
	void setROI(const BoundingBox& roi);

	void setSvs();

	void print();
	int getOutputExtension() const;
	void setOutputExtension(int outputExtension);
	std::string getInputFileName() const;
	void setInputFileName(std::string inputFileName);
	bool getIsAppInput() const;
	void setIsAppInput(bool isAppInput);
	long getCachedDataSize() const;
	void setCachedDataSize(long cachedDataSize);
	int getStorageLevel() const;
	void setStorageLevel(int storageLevel);
};


#endif /* DATA_H_ */
