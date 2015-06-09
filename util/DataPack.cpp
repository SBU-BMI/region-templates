/*
 * DataPack.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: george
 */

#include "DataPack.h"

DataPack::DataPack() {
}

DataPack::~DataPack() {
}

void DataPack::unpack(char* msg, int& deserialized_bytes,
		std::string& retValue) {
	int strSize = ((int*)(msg+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// unpack id
	char *str = new char[strSize+1];
	str[strSize] = '\0';
	memcpy(str, msg+deserialized_bytes, sizeof(char)*strSize);
	deserialized_bytes += sizeof(char)*strSize;
	retValue = str;
	delete str;
}

void DataPack::unpack(char* msg, int& deserialized_bytes, int& retValue) {
	retValue = ((int*)(msg+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);
}

void DataPack::pack(char* msg, int& serialized_bytes, std::string input) {
	int strSize = input.size();
	memcpy(msg+serialized_bytes, &strSize, sizeof(int));
	serialized_bytes += sizeof(int);

	memcpy(msg+serialized_bytes, input.c_str(), strSize*sizeof(char));
	serialized_bytes += strSize*sizeof(char);
}

void DataPack::pack(char* msg, int& serialized_bytes, int input) {
	memcpy(msg+serialized_bytes, &input, sizeof(int));
	serialized_bytes += sizeof(int);
}

int DataPack::packSize(std::string input) {
	int dataSize = sizeof(int);
	dataSize += input.size();
	return dataSize;
}
