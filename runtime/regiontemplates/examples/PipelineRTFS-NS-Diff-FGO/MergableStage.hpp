/*
 * MergableStage.hpp
 *
 *  Created on: Jul 18, 2016
 *      Author: willian
 */

#ifndef MERGABLE_STAGE_H_
#define MERGABLE_STAGE_H_

class MergableStage {

public:
	MergableStage() {};

	virtual void merge(MergableStage &s) = 0;
};

#endif /* MERGABLE_STAGE_H_ */
