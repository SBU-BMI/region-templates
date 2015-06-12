/*
 * CachePolicy.cpp
 *
 *  Created on: Aug 26, 2014
 *      Author: george
 */

#include "CachePolicy.h"

CachePolicy::CachePolicy() {
}

CachePolicy::~CachePolicy() {
}

// this operations does not affect order in FIFO policy
void CachePolicy::accessItem(DRKey id) {
	return;
}

// remove from the front of the queue
DRKey CachePolicy::selectDelete(void) {
	DRKey ret;
	if(this->accessList.size() > 0){
		// get id of the first element in the queue
		ret = this->accessList.front();
		// delete that element
		this->accessList.pop_front();
	}
	return ret;
}

// insert into the back of the queue
void CachePolicy::insertItem(DRKey id) {
	this->accessList.push_back(id);
}

int CachePolicy::size() {
	return this->accessList.size();
}

// Data class used to store data region key.
DRKey::DRKey() {
}

DRKey::DRKey(std::string rtName, std::string rtId, std::string drName, std::string drId,
		int timeStamp, int version) {
	this->setRtName(rtName);
	this->setRtId(rtId);
	this->setDrName(drName);
	this->setDrId(drId);
	this->setTimeStamp(timeStamp);
	this->setVersion(version);
}

DRKey::~DRKey() {
}

std::string DRKey::getDrName() const {
	return drName;
}

void DRKey::setDrName(std::string drName) {
	this->drName = drName;
}

std::string DRKey::getRtId() const {
	return rtId;
}

void DRKey::setRtId(std::string rtId) {
	this->rtId = rtId;
}

std::string DRKey::getRtName() const {
	return rtName;
}

void DRKey::setRtName(std::string rtName) {
	this->rtName = rtName;
}

int DRKey::getTimeStamp() const {
	return timeStamp;
}

void DRKey::setTimeStamp(int timeStamp) {
	this->timeStamp = timeStamp;
}

int DRKey::getVersion() const {
	return version;
}

void DRKey::setVersion(int version) {
	this->version = version;
}

bool DRKey::isEqual(DRKey element) {
	bool retValue = false;
	if(this->getDrId().compare(element.getDrId())==0 && this->getDrName().compare(element.getDrId()) == 0 &&
			this->getRtId().compare(element.getRtId())== 0 && this->getRtName().compare(element.getRtName()) == 0 &&
			this->getTimeStamp() == element.getTimeStamp() && this->getVersion() == element.getVersion()){
		retValue = true;
	}
	return retValue;
}

void DRKey::print() {
	std::cout<< "rtName: "<< this->getRtName() << " rtId: " << this->getRtId() << " drName: "<< this->getDrName() <<
		" drId: "<< this->getDrId() <<" timeStamp: " << this->getTimeStamp() << " version: "<< this->getVersion() << std::endl;
}

std::string DRKey::getDrId() const {
	return drId;
}

void DRKey::setDrId(std::string drId) {
	this->drId = drId;
}
