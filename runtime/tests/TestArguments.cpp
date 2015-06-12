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

	ArgumentFloatArray floatAr;

	floatAr.addArgValue(ArgumentFloat(-0.23));
	floatAr.addArgValue(ArgumentFloat(1.23));
	floatAr.addArgValue(ArgumentFloat(0.54));

	std::cout << "FloatArray: size: "<< floatAr.size() << std::endl;
	char *buff2[floatAr.size()];
	number_bytes_serialized = floatAr.serialize((char*)buff2);
	std::cout << "FloatArray: number_bytes_serialized="<< number_bytes_serialized <<std::endl;
	assert(number_bytes_serialized == floatAr.size());

	ArgumentFloatArray floatAr2;
	int deserialize = floatAr2.deserialize((char*)buff2);
	std::cout << "FloatArray: deserialized size: "<< deserialize << std::endl;

	for(int i = 0; i < floatAr2.getNumArguments(); i++){
		float aux = floatAr2.getArgValue(i).getArgValue();
		std::cout << " Arg i: "<< i <<" = "<< aux;
	}
	std::cout<<std::endl;

	return 0;
}


