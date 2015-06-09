/*
 * CompA.h
 *
 *  Created on: Feb 16, 2012
 *      Author: george
 */

#ifndef COMPA_H_
#define COMPA_H_

#include "PipelineComponentBase.h"

class CompA : public PipelineComponentBase {
private:
	int b, c, k1, k2, aOut, bOut, cOut;

public:
	CompA();
	virtual ~CompA();

	int run();

};

#endif /* COMPA_H_ */
