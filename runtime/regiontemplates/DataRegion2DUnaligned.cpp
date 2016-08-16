/*
 * DataRegion2DUnaligned.cpp
 *
 *  Created on: Aug 12, 2016
 *      Author: george
 */

#include "DataRegion2DUnaligned.h"

DataRegion2DUnaligned::DataRegion2DUnaligned() {
	this->setType(DataRegionType::REGION_2D_UNALIGNED);
}


DataRegion2DUnaligned::~DataRegion2DUnaligned() {
}

DataRegion* DataRegion2DUnaligned::clone(bool copyData) {
	DataRegion2DUnaligned* clonedDataRegion = new DataRegion2DUnaligned();

	// First, copy information stored into the DataRegion father's class.
	// This is done by serializing that class and deserializing it into
	// the "cloned" data region.

	// get size need to serialize fathers class
	int serializeSize = this->serializationSize();

	// create serialization buffer
	char* serBuffer = (char*)malloc(sizeof(char)* serializeSize);

	// serialize fathers class
	this->serialize(serBuffer);

	// deserialize information using the "clone object"
	clonedDataRegion->deserialize(serBuffer);
	// release buffer
	free(serBuffer);

	// copy data as well if copyData is set
	if(copyData){
		clonedDataRegion->data = this->data;
	}else{
		//clonedDataRegion->data = &this->data;
	}

	return clonedDataRegion;
}

bool DataRegion2DUnaligned::empty() {
	return (this->data.size()==0);
}

long DataRegion2DUnaligned::getDataSize() {
	long dataSize = 0;
	for(int i = 0; i < this->data.size(); i++){
		dataSize += this->data[i].size() * sizeof(double);
	}
	return dataSize;
}

std::vector<std::vector<double> >& DataRegion2DUnaligned::getData() {
	return data;
}

void DataRegion2DUnaligned::setData(std::vector< std::vector<double> > &data) {
	this->data = data;
	this->cachedDataSize = this->getDataSize();
}

bool DataRegion2DUnaligned::compare(DataRegion2DUnaligned* dr) {
	bool retValue = true;
	int numWrong = 0;

	// Make sure we have the same number of vectors in both data regions
	if(this->data.size() != dr->data.size()){
		retValue = false;

	}else{
		// compare all vectors
		for(int i = 0; i < this->data.size() && retValue == true; i++){
			if(this->data[i].size() != dr->data[i].size()){
				retValue = false;
				break;
			}
			// compare elements in ith vector
			for(int j = 0; j < this->data[i].size(); j++){
				if(this->data[i][j] != dr->data[i][j]){
					retValue = false;
					numWrong++;
					break;
				}
			}
		}
	}
	return retValue;
}
