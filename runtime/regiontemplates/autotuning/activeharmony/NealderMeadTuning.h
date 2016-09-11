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

class NealderMeadTuning : public TuningInterface {
private:
    int harmonySessionStatus;
    static const int numClients = 1;
    TuningParamSet tuningParamSet[numClients];
    std::vector<hdesc_t *> hdesc;
    char name[1024];

    int initialize(int argc, char **argv);

    int bindParam(std::string paramLabel, int setId = 0);

public:
    NealderMeadTuning(int argc, char **argv);

    int declareParam(std::string paramLabel, void *paramLowerBoundary, void *paramHigherBoundary, void *paramStepSize,
                     int setId = 0);

    int configure(int strategy, int maxNumberOfIterations);

    int nextIteration();

    TuningParamSet *fetchParamSet(int setId);

    int reportScore(void *scoreValue, int setId);

    bool hasConverged();

    int getHarmonySessionStatus() const {
        return harmonySessionStatus;
    }


};


#endif //RUNTIME_NEALDERMEADTUNING_H
