/*
 * CompB.h
 *
 *  Created on: Feb 16, 2012
 *      Author: george
 */

#ifndef COMPB_H_
#define COMPB_H_

#include "PipelineComponentBase.h"

class CompB : public PipelineComponentBase {
private:
	int b, c, k1, k2, aOut, bOut, cOut, dOut;

public:
	CompB();
	virtual ~CompB();

	int run();

};

#endif /* COMPB_H_ */
