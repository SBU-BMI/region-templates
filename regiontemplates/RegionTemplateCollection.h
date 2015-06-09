/*
 * RegionTemplateCollection.h
 *
 *  Created on: Feb 13, 2013
 *      Author: george
 */

#ifndef REGIONTEMPLATECOLLECTION_H_
#define REGIONTEMPLATECOLLECTION_H_

#include "RegionTemplate.h"

class RegionTemplateCollection {
private:
	std::string name;
	std::vector<RegionTemplate *> rts;
public:
	RegionTemplateCollection();
	virtual ~RegionTemplateCollection();

	int addRT(RegionTemplate *rt);
	RegionTemplate* getRT(int index);
	int getNumRTs();

	std::string getName() const {
		return name;
	}

	void setName(std::string name) {
		this->name = name;
	}
};

#endif /* REGIONTEMPLATECOLLECTION_H_ */
