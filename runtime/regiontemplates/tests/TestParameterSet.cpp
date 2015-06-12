/*
 * TestArguments.cpp
 *
 *  Created on: Feb 20, 2012
 *      Author: george
 */
#include <assert.h>

#include "Argument.h"
#include "ParameterSet.h"


int main(int argc, char **argv){
	vector<ArgumentBase*> argSetInstance;

	ParameterSet parSet;
	argSetInstance = parSet.getNextArgumentSetInstance();
	std::cout << "argSetInstance size: "<< argSetInstance.size() << std::endl;

	parSet.addRangeArguments(1, 2, 1);
	parSet.addRangeArguments(1, 3, 1);
	parSet.addRangeArguments(1, 4, 1);

	argSetInstance = parSet.getNextArgumentSetInstance();

	while((argSetInstance).size() != 0){
		int value1 = ((ArgumentInt*)argSetInstance[0])->getArgValue();
		int value2 = ((ArgumentInt*)argSetInstance[1])->getArgValue();
		int value3 = ((ArgumentInt*)argSetInstance[2])->getArgValue();
		for(int i = 0; i < 3; i++)
			delete argSetInstance[i];
		argSetInstance = parSet.getNextArgumentSetInstance();
		int nextSize = argSetInstance.size();
		//argSetInstance = parSet.getNextArgumentSetInstance();
		std::cout << "Value1: "<< value1<< " value2: "<< value2 << " value3: "<< value3 << " NextSize: "<< nextSize<<std::endl;
	}


	vector<ArgumentBase*> floatArg;
	floatArg.push_back(new ArgumentFloat(1.3));
	floatArg.push_back(new ArgumentFloat(2.3));
	floatArg.push_back(new ArgumentFloat(2.4));
	parSet.addArguments(floatArg);

	parSet.resetIterator();
	argSetInstance = parSet.getNextArgumentSetInstance();

	while((argSetInstance).size() != 0){
		int value1 = ((ArgumentInt*)argSetInstance[0])->getArgValue();
		int value2 = ((ArgumentInt*)argSetInstance[1])->getArgValue();
		int value3 = ((ArgumentInt*)argSetInstance[2])->getArgValue();
		float value4 = ((ArgumentFloat*)argSetInstance[3])->getArgValue();
		for(int i = 0; i < 4; i++)
			delete argSetInstance[i];

		argSetInstance = parSet.getNextArgumentSetInstance();
		int nextSize = argSetInstance.size();
		//argSetInstance = parSet.getNextArgumentSetInstance();
		std::cout << "Value1: "<< value1<< " value2: "<< value2 << " value3: "<< value3 << " value4: " << value4<< " NextSize: "<< nextSize<<std::endl;
	}

	vector<ArgumentBase*> stringArg;
	stringArg.push_back(new ArgumentString("First"));
	stringArg.push_back(new ArgumentString("Second"));
	stringArg.push_back(new ArgumentString("Third"));
	parSet.addArguments(stringArg);

	parSet.resetIterator();
	argSetInstance = parSet.getNextArgumentSetInstance();

	while((argSetInstance).size() != 0){
		int value1 = ((ArgumentInt*)argSetInstance[0])->getArgValue();
		int value2 = ((ArgumentInt*)argSetInstance[1])->getArgValue();
		int value3 = ((ArgumentInt*)argSetInstance[2])->getArgValue();
		float value4 = ((ArgumentFloat*)argSetInstance[3])->getArgValue();
		string value5 = ((ArgumentString*)argSetInstance[4])->getArgValue();

		for(int i = 0; i < 5; i++)
			delete argSetInstance[i];

		argSetInstance = parSet.getNextArgumentSetInstance();
		int nextSize = argSetInstance.size();
		//argSetInstance = parSet.getNextArgumentSetInstance();
		std::cout << "Value1: "<< value1<< " value2: "<< value2 << " value3: "<< value3 << " value4: " << value4<< " value5: " << value5<< " NextSize: "<< nextSize<<std::endl;
	}

	return 0;
}


