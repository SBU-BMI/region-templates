/*
 * BasicDataSource.h
 *
 *  Created on: Oct 22, 2012
 *      Author: george
 */

#ifndef BASICDATASOURCE_H_
#define BASICDATASOURCE_H_

#include "BoundingBox.h"
//#include "DataRegion.h"

class DataRegion;

class BasicDataSource {
private:
	std::string name;
	int type;

public:
	BasicDataSource();
	virtual ~BasicDataSource();

	virtual bool instantiateRegion(DataRegion *dataRegion) {return false;};
	virtual bool writeRegion(DataRegion *dataRegion) {return false;};


	int getType() const;
	void setType(int type);
	std::string getName() const;
	void setName(std::string name);
};

#endif /* BASICDATASOURCE_H_ */
