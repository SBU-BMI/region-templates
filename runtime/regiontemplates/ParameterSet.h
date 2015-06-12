/*
 * ParameterSet.h
 *
 *  Created on: Jan 7, 2015
 *      Author: george
 */

#ifndef PARAMETERSET_H_
#define PARAMETERSET_H_

#include "Argument.h"
#include <vector>

using namespace std;

class ParameterSet {
	std::vector<std::vector<ArgumentBase*> > argumentSet;
	std::vector<int> iterator;

public:
	ParameterSet();
	virtual ~ParameterSet();

	void addArguments(std::vector<ArgumentBase*> argValues);
	void addArgument(ArgumentBase* argValue);
	void addRangeArguments(int begin, int end, int step);
	vector<ArgumentBase*> getNextArgumentSetInstance();
	void resetIterator();

};

#endif /* PARAMETERSET_H_ */
