/*
 * DenseDataRegion3D.h
 *
 *  Created on: Oct 22, 2012
 *      Author: george
 */

#ifndef DENSEDATAREGION3D_H_
#define DENSEDATAREGION3D_H_

#include <vector>
#include "DataRegion.h"
#include "DenseDataRegion2D.h"

class DenseDataRegion3D: public DataRegion {
private:
	std::string name;
	std::vector<DenseDataRegion2D *> data;

public:
	DenseDataRegion3D();
	virtual ~DenseDataRegion3D();
};

#endif /* DENSEDATAREGION3D_H_ */
