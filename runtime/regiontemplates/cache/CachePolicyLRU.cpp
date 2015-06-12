/*
 * CachePolicyLRU.cpp
 *
 *  Created on: Jan 6, 2015
 *      Author: george
 */

#include "CachePolicyLRU.h"

CachePolicyLRU::CachePolicyLRU() {

}

CachePolicyLRU::~CachePolicyLRU() {

}

void CachePolicyLRU::insertItem(DRKey id) {
	for (std::list<DRKey>::iterator it=this->accessList.begin(); it != accessList.end(); ++it){
		DRKey current = *it;
		// if already stored
		if(id.isEqual(current)){
			this->accessList.erase(it);
		}
	}
	this->accessList.push_back(id);
	return;
}

void CachePolicyLRU::accessItem(DRKey id) {
	this->insertItem(id);
}

