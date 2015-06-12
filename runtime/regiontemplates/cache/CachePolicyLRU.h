/*
 * CachePolicyLRU.h
 *
 *  Created on: Jan 6, 2015
 *      Author: george
 */

#ifndef CACHEPOLICYLRU_H_
#define CACHEPOLICYLRU_H_

#include "CachePolicy.h"

class CachePolicyLRU: public CachePolicy {

public:
	CachePolicyLRU();
	virtual ~CachePolicyLRU();

	// item was inserted
	void insertItem(DRKey id);
	// inform that item was accessed
	void accessItem(DRKey id);

};

#endif /* CACHEPOLICYLRU_H_ */
