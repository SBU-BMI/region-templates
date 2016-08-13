//
// Created by taveira on 8/8/16.
//

#ifndef RUNTIME_TUNINGINTERFACE_H
#define RUNTIME_TUNINGINTERFACE_H

class TuningInterface {

protected:
    long iteration;
    long amountOfParamSetsPerIteration; //Amount of param sets tested each iteration
    TuningParamSet **paramSet;

public:

    virtual bool hasConverged() = 0;

    virtual void reportParamSet(TuningParamSet paramSet, int setId = 0) = 0;

    virtual TuningParamSet fetchNewParamSet(int setId = 0) = 0;

    virtual nextIteration() = 0;

    long getIteration();

};

long TuningParamSet::getIteration() {
    return this.iteration;
}

#endif //RUNTIME_TUNINGINTERFACE_H
