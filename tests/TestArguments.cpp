/*
 * TestArguments.cpp
 *
 *  Created on: Feb 20, 2012
 *      Author: george
 */
#include <assert.h>

#include "Argument.h"


int main(int argc, char **argv){

	ArgumentString as("string");

	std::cout << "as.value="<<as.getArgValue()<<std::endl;
	std::cout << "as.size="<<as.size()<<std::endl;

	// auxiliar variable used to serialize as.
	char *buff[as.size()];
	int number_bytes_serialized = as.serialize((char*)buff);

	std::cout << "number_bytes_serialized="<< number_bytes_serialized <<std::endl;
	assert(number_bytes_serialized == as.size());

	ArgumentString bs;
	bs.deserialize((char*)buff);
	std::cout << "bs.value=" << as.getArgValue() <<std::endl;

	return 0;
}


