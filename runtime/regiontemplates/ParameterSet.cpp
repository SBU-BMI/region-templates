/*
 * ParameterSet.cpp
 *
 *  Created on: Jan 7, 2015
 *      Author: george
 */

#include "ParameterSet.h"

ParameterSet::ParameterSet() {

}

ParameterSet::~ParameterSet() {
	for(int i = 0; i < this->argumentSet.size(); i++){
		for(int j = 0; j < this->argumentSet[i].size(); j++){
			delete argumentSet[i][j];
		}
	}
}

void ParameterSet::addArgument(ArgumentBase* argValue) {
	std::vector<ArgumentBase*> auxVec;
	auxVec.push_back(argValue);
	this->addArguments(auxVec);
}
void ParameterSet::addArguments(std::vector<ArgumentBase*> argValues) {
	this->argumentSet.push_back(argValues);
}

void ParameterSet::addRangeArguments(int begin, int end, int step) {
	std::vector<ArgumentBase*> arg;
	for(int curr = begin; curr <= end; curr+=step){
		ArgumentInt *aux = new ArgumentInt(curr);
		arg.push_back(aux);
	}
	this->argumentSet.push_back(arg);
}

vector<ArgumentBase*> ParameterSet::getNextArgumentSetInstance() {
	vector<ArgumentBase*> arg;

	if(this->argumentSet.size() == 0) return arg;
	// start iterator
	if(this->argumentSet.size() != this->iterator.size()){
		resetIterator();
	}

	// if it has not iterated until the very end
	if(this->argumentSet[0].size() != this->iterator[0]){
		// init return arg set
		for(int i = 0; i < this->argumentSet.size(); i++){
			arg.push_back((argumentSet[i][iterator[i]])->clone());
		}

		// update iterator
		for(int i = this->argumentSet.size()-1; i >= 0; i--){
			this->iterator[i]++;
			if(this->iterator[i] == this->argumentSet[i].size() && i > 0){
				this->iterator[i] = 0;
			}else{
				break;
			}
		}
	}
	return arg;
}

void ParameterSet::resetIterator() {

	if(this->argumentSet.size() != this->iterator.size()){
		while(this->argumentSet.size() != this->iterator.size()){
			this->iterator.push_back(0);
		}
	}

	for(int i = 0; i < this->iterator.size(); i++){
		iterator[i] = 0;
	}
}


