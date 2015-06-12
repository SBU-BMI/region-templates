/*
 * DataPack.h
 *
 *  Created on: Sep 10, 2014
 *      Author: george
 */

#ifndef DATAPACK_H_
#define DATAPACK_H_

#include <string.h> // memcpy
#include <string> // std::string

class DataPack {
private:
	DataPack();
	virtual ~DataPack();
public:

	static void unpack(char *msg, int &deserialized_bytes, std::string& retValue);
	static void unpack(char *msg, int &deserialized_bytes, int& retValue);

	static void pack(char *msg, int &serialized_bytes, std::string input);
	static void pack(char *msg, int &serialized_bytes, int input);

	static int packSize(std::string input);
};

#endif /* DATAPACK_H_ */
