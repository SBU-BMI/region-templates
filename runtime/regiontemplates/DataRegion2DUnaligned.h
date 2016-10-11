/*
 * DataRegion2DUnaligned.h
 *
 *  Created on: Aug 12, 2016
 *      Author: george
 */

#ifndef DATAREGION2DUNALIGNED_H_
#define DATAREGION2DUNALIGNED_H_

#include <vector>
#include <iostream>
#include "DataRegion.h"

class DataRegion2DUnaligned: public DataRegion {
private:


public:
	std::vector<std::vector<double> > data;

    std::vector<std::string> featureLabels;

	DataRegion2DUnaligned();
	virtual ~DataRegion2DUnaligned();

	DataRegion* clone(bool copyData);
	bool empty();
	long getDataSize();
	std::vector<std::vector<double> > &getData();
	void setData(std::vector< std::vector<double> > &data);

	bool compare(DataRegion2DUnaligned* dr);

};


#endif /* DATAREGION2DUNALIGNED_H_ */
