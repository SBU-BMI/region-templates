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
    int iteration;
    int maxNumberOfIterations;
    TuningParamSet **tuningParamSet;


    virtual int bindParam(std::string paramLabel, int setId = 0) = 0;

public:

    virtual bool hasConverged() = 0;

    virtual int declareParam(std::string paramLabel, double paramLowerBoundary, double paramHigherBoundary,
                             double paramStepSize,
                             int setId = 0) = 0;

    virtual int initialize(int argc, char **argv) = 0;
    virtual int configure() = 0;

    virtual int fetchParams() = 0;

    virtual TuningParamSet *getParamSet(int setId = 0) = 0;

    virtual int reportScore(double scoreValue, int setId) = 0;

    virtual void nextIteration() = 0;

    int getIteration();

};

int TuningInterface::getIteration() {
    return this->iteration;
}

#endif //RUNTIME_TUNINGINTERFACE_H
