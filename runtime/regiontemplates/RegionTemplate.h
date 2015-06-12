/*
 * RegionTemplate.h
 *
 *  Created on: Oct 22, 2012
 *      Author: george
 */

#ifndef REGIONTEMPLATE_H_
#define REGIONTEMPLATE_H_

#include <list>
#include <string>
#include <iostream>
#include "DataRegion.h"
#include "DenseDataRegion2D.h"
#include "BoundingBox.h"
#include "BasicDataSource.h"
#include "Constants.h"
#include "Cache.h"
#include "PipelineComponentBase.h"
#include "Util.h"

class RegionTemplate {
private:

	// Data elements. It might be 2D/3D arrays representing image or mesh, or ...
	std::map<std::string, std::list<DataRegion*> > templateRegions;

	// Name of this data template region
	std::string name;

	// Id of this specific instance
	std::string id;

	// Version of the data
	int version;

	// If there is a temporal relationship
	int timestamp;

	// (x,y,z) coordinates represented by this region template
	BoundingBox bb;

	// Pointer to cache infrastructure stored w/ Worker
	Cache *cachePtr;

	// if true, data is not automatically read when component is created
	bool lazyRead;

	// is it at the worker or manager side?
	int location;

	friend class RTPipelineComponentBase;
protected:
	void setCache(Cache *cache);
	int getLocation() const;
	void setLocation(int location);

public:
	RegionTemplate();
	virtual ~RegionTemplate();
	// TODO: move back to protected area
	Cache* getCache(void);
	// TODO... change region bounding box if necessary.
	bool insertDataRegion(DataRegion *dataRegion);
	DataRegion *getDataRegion(std::string drName, std::string drId="", int timestamp=0, int version=0);
	bool deleteDataRegion(std::string drName, std::string drId="", int timestamp=0, int version=0);

	int getDataRegionListSize(std::string name);

	int getNumDataRegions();
	DataRegion *getDataRegion(int index);

	const BoundingBox& getBb() const;
	void setBb(const BoundingBox& bb);
	std::string getName() const;
	void setName(std::string name);
	unsigned int getVersion() const;
	void setVersion(unsigned int version);

	void instantiateRegion();
	int getTimestamp() const;
	void setTimestamp(int timestamp);

	int serialize(char *buff);
	int deserialize(char *buff);
	int size();
	std::string getId() const;
	void setId(std::string id);

	void print();
	bool isLazyRead() const;
	void setLazyRead(bool lazyRead);

};

#ifdef MATLAB_INTEGRATION
//	DataRegion *getDataRegion(std::string name, int version=0, int timestamp=0);

#endif


#endif /* REGIONTEMPLATE_H_ */
