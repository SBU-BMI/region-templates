//
// Created by taveira on 8/8/16.
//

#ifndef RUNTIME_TUNINGINTERFACE_H
#define RUNTIME_TUNINGINTERFACE_H

#include "TuningParamSet.h"

#include <unistd.h>
#include <stddef.h>
#include <iostream>
#include <vector>

class TuningInterface;
class TuningInterface {

protected:
    long iteration;
    long maxNumberOfIterations;
    long amountOfParamSetsPerIteration; //Amount of param sets tested each iteration
    TuningParamSet **paramSet;

    virtual int bindParam(std::string paramLabel, int setId = 0) = 0;

public:

    virtual bool hasConverged() = 0;

    virtual int declareParam(std::string paramLabel, void *paramLowerBoundary, void *paramHigherBoundary,
                             void *paramStepSize, int setId = 0) = 0;

    virtual TuningParamSet *fetchParamSet(int setId = 0) = 0;

    virtual int reportScore(void *scoreValue, int setId) = 0;

    virtual int nextIteration() = 0;

    long getIteration();

};

long TuningInterface::getIteration() {
    return this->iteration;
}

#endif //RUNTIME_TUNINGINTERFACE_H
