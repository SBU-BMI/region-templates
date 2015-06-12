/*
 * FireTrackingComponent.h
 *
 *  Created on: Feb 7, 2013
 *  Author: george
 */

#ifndef FIRE_TRACKING_COMPONENT_H_
#define FIRE_TRACKING_COMPONENT_H_

#include "RTPipelineComponentBase.h"
#include "TaskTracking.h"

class FireTrackingComponent : public RTPipelineComponentBase {
private:

public:
	FireTrackingComponent();
	virtual ~FireTrackingComponent();

	int run();

};

#endif /* FIRE_TRACKING_COMPONENT_H_ */
