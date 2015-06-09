/*
 * BoundingBox.h
 *
 *  Created on: Oct 22, 2012
 *      Author: george
 */

#ifndef BOUNDINGBOX_H_
#define BOUNDINGBOX_H_

#include <iostream>
#include <string.h>



class Point{
private:
	int x, y, z;
public:
	Point(){};
	Point(int x, int y, int z);
	~Point(){};

	int getX() const;
	void setX(int x);
	int getY() const;
	void setY(int y);
	int getZ() const;
	void setZ(int z);
};
class BoundingBox {
private:
	// lower dimension values corner
	Point lb;
	// upper dimension values corner
	Point ub;

public:
	BoundingBox();
	BoundingBox(Point lb, Point ub);
	virtual ~BoundingBox();



	unsigned int sizeCoordX();
	unsigned int sizeCoordY();
	unsigned int sizeCoordZ();

	bool doesIntersect(BoundingBox b);
	BoundingBox intersection(BoundingBox b);


	void print() const;
	const Point& getLb() const;
	const Point& getUb() const;

	int serialize(char *buff);
	int deserialize(char* buff);
	int size();

	void setLb(const Point& lb);
	void setUb(const Point& ub);
};

struct BBComparator{
	bool operator()(const BoundingBox &bb1, const BoundingBox &bb2){

		if(bb1.getLb().getX() < bb2.getLb().getX()){
			return true;
		}else{
			if(bb1.getLb().getX() == bb2.getLb().getX()){
				if(bb1.getLb().getY() < bb2.getLb().getY()){
					return true;
				}else{
					if(bb1.getLb().getY() == bb2.getLb().getY()){
						if(bb1.getLb().getZ() < bb2.getLb().getZ()){
							return true;
						}
					}
				}
			}
		}
		return false;
	}
};
#endif /* BOUNDINGBOX_H_ */
