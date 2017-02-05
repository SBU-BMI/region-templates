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
    int numSets;

    TuningParamSet bestSet;

public:

    virtual bool hasConverged() = 0;

    virtual int declareParam(std::string paramLabel, double paramLowerBoundary, double paramHigherBoundary,
                             double paramStepSize) = 0;

    virtual int initialize(int argc, char **argv) = 0;
    virtual int configure() = 0;

    virtual int fetchParams() = 0;

    virtual TuningParamSet *getParamSet(int setId = 0) = 0;

    virtual double getParamValue(std::string paramName, int setId = 0) = 0;

    virtual int reportScore(double scoreValue, int setId) = 0;

    virtual void nextIteration() = 0;

    int getIteration() {
        return this->iteration;
    }

    TuningParamSet *getBestParamSet() {
        return &bestSet;
    }
};


#endif //RUNTIME_TUNINGINTERFACE_H
