/*
 * BoundingBox.cpp
 *
 *  Created on: Oct 22, 2012
 *      Author: george
 */

#include "BoundingBox.h"

BoundingBox::BoundingBox() {

}

BoundingBox::BoundingBox(Point lb, Point ub) {
	this->ub = ub;
	this->lb = lb;
}

BoundingBox::~BoundingBox() {

}


int Point::getX() const {
	return x;
}

void Point::setX(int x) {
	this->x = x;
}

int Point::getY() const {
	return y;
}

void Point::setY(int y) {
	this->y = y;
}

int Point::getZ() const {
	return z;
}

Point::Point(int x, int y, int z) {
	this->setX(x);
	this->setY(y);
	this->setZ(z);
}

void Point::setZ(int z) {
	this->z = z;
}

unsigned int BoundingBox::sizeCoordX() {
	return (ub.getX() - lb.getX()  + 1);
}

unsigned int BoundingBox::sizeCoordY() {
	return (ub.getY() - lb.getY() + 1);
}

unsigned int BoundingBox::sizeCoordZ() {
	return (ub.getZ() - lb.getZ() + 1);
}

bool interset1D(int lb0, int lb1, int ub0, int ub1){
	if(	(lb0 <= lb1 && lb1 <= ub0) ||
		(lb1 <= lb0 && lb0 <= ub1)){
		return true;
	}
	return false;
}
bool BoundingBox::doesIntersect(BoundingBox b) {
	if(	interset1D(this->getLb().getX(), b.getLb().getX(), this->getUb().getX(), b.getUb().getX()) &&
		interset1D(this->getLb().getY(), b.getLb().getY(), this->getUb().getY(), b.getUb().getY()) &&
		interset1D(this->getLb().getZ(), b.getLb().getZ(), this->getUb().getZ(), b.getUb().getZ()) ){
			return true;
	}
	return false;
}

const Point& BoundingBox::getLb() const {
return lb;
}

BoundingBox BoundingBox::intersection(BoundingBox b) {
	Point lb, ub;
	lb.setX(std::max(this->getLb().getX(), b.getLb().getX()));
	lb.setY(std::max(this->getLb().getY(), b.getLb().getY()));
	lb.setZ(std::max(this->getLb().getZ(), b.getLb().getZ()));

	ub.setX(std::min(this->getUb().getX(), b.getUb().getX()));
	ub.setY(std::min(this->getUb().getY(), b.getUb().getY()));
	ub.setZ(std::min(this->getUb().getZ(), b.getUb().getZ()));
	BoundingBox intersectBB(lb, ub);
	return intersectBB;
}


const Point& BoundingBox::getUb() const {
	return ub;
}

void BoundingBox::print() const{
	std::cout << "	lb(" << this->lb.getX() << "," << this->lb.getY() << "," << this->lb.getZ() << "), ";
	std::cout << "	ub(" << this->ub.getX() << "," << this->ub.getY() << "," << this->ub.getZ() << ")";
}

int BoundingBox::serialize(char* buff) {
	int serialized_bytes = 0;
	// Pack bounding box info (LB: lower dimensions, UB:  upper dimensions)
	Point lb = this->getLb();
	int x = lb.getX(), y = lb.getY(), z = lb.getZ();
	memcpy(buff+serialized_bytes, &x, sizeof(int));
	serialized_bytes += sizeof(int);

	memcpy(buff+serialized_bytes, &y, sizeof(int));
	serialized_bytes += sizeof(int);

	memcpy(buff+serialized_bytes, &z, sizeof(int));
	serialized_bytes += sizeof(int);

	Point ub = this->getUb();
	x = ub.getX(); y = ub.getY(); z = ub.getZ();
	memcpy(buff+serialized_bytes, &x, sizeof(int));
	serialized_bytes += sizeof(int);

	memcpy(buff+serialized_bytes, &y, sizeof(int));
	serialized_bytes += sizeof(int);

	memcpy(buff+serialized_bytes, &z, sizeof(int));
	serialized_bytes += sizeof(int);

	return serialized_bytes;
}

int BoundingBox::deserialize(char* buff) {
	int deserialized_bytes = 0;

	// Extract lb point
	// extract BB X dimension
	int x =((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract BB Y dimension
	int y =((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract BB Z dimension
	int z =((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);
	Point lb(x, y, z);
	this->setLb(lb);

	// Extract lb point
	// extract BB X dimension
	x =((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract BB Y dimension
	y =((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract BB Z dimension
	z =((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);
	Point ub(x, y, z);
	this->setUb(ub);

	return deserialized_bytes;
}

void BoundingBox::setLb(const Point& lb) {
	this->lb = lb;
}

void BoundingBox::setUb(const Point& ub) {
	this->ub = ub;
}

int BoundingBox::size() {
	return sizeof(int)*6;
}
