/*
 * RegionTemplateCollection.cpp
 *
 *  Created on: Feb 13, 2013
 *      Author: george
 */

#include "RegionTemplateCollection.h"

RegionTemplateCollection::RegionTemplateCollection() {

}

RegionTemplateCollection::~RegionTemplateCollection() {
	for(int i = 0; i < this->getNumRTs(); i++){
		delete rts[i];
	}
	rts.clear();
}

int RegionTemplateCollection::addRT(RegionTemplate* rt) {
	rts.push_back(rt);
	return 1;
}

RegionTemplate* RegionTemplateCollection::getRT(int index) {
	RegionTemplate* rtReturn = NULL;
	if(index >=0 && index < this->getNumRTs()){
		rtReturn = rts[index];
	}
	return rtReturn;
}

int RegionTemplateCollection::getNumRTs() {
	return this->rts.size();
}
