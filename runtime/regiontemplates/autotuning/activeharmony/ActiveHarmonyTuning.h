//
// Created by taveira on 8/31/16.
//

#ifndef RUNTIME_NEALDERMEADTUNING_H
#define RUNTIME_NEALDERMEADTUNING_H

#include "../TuningInterface.h"
#include "../TuningParamSet.h"
#include "Util.h"
#include "hclient.h"
#include <stdlib.h>

class ActiveHarmonyTuning : public TuningInterface {
private:
    int harmonySessionStatus;
    std::string strategyAHpolicy;
    std::vector<hdesc_t *> hdesc;
    char name[1024];
    std::string AHpolicy;

    int bindParam(std::string paramLabel, int setId = 0);

public:
    ActiveHarmonyTuning(std::string strategy, int maxNumberOfIterations, int numSets);

    int initialize(int argc, char **argv);
    int declareParam(std::string paramLabel, double paramLowerBoundary, double paramHigherBoundary,
                     double paramStepSize);

    int configure();

    void nextIteration();

    int fetchParams();

    TuningParamSet *getParamSet(int setId = 0);

    double getParamValue(std::string paramName, int setId = 0);

    int reportScore(double scoreValue, int setId);

    bool hasConverged();


};


#endif //RUNTIME_NEALDERMEADTUNING_H
