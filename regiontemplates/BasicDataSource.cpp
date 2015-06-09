/*
 * BasicDataSource.cpp
 *
 *  Created on: Oct 22, 2012
 *      Author: george
 */

#include "BasicDataSource.h"

BasicDataSource::BasicDataSource() {

}

BasicDataSource::~BasicDataSource() {
}

int BasicDataSource::getType() const {
	return type;
}

std::string BasicDataSource::getName() const {
	return name;
}

void BasicDataSource::setName(std::string name) {
	this->name = name;
}

void BasicDataSource::setType(int type) {
	this->type = type;
}
