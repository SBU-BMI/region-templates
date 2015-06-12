/*
 * CompPrint.h
 *
 *  Created on: Feb 16, 2012
 *      Author: george
 */

#ifndef COMPPRINT_H_
#define COMPPRINT_H_

#include "PipelineComponentBase.h"

class CompPrint : public PipelineComponentBase {
public:
	CompPrint();
	virtual ~CompPrint();

	int run();

};

#endif /* COMPPRINT_H_ */
