//
// Created by taveira on 11/22/15.
//

#include "RandomInitialization.h"

void RandomInitialization::initializePopulationAttributes(GAIndividual *pop, GAIndividual *nextGenPop, int popsize) {
#ifdef _OPENMP
#pragma omp for schedule (dynamic)
#endif
    for (int i = 0; i < popsize; i++) {
        randAnIndividual(&pop[i], amountOfDeclaredParams);
        randAnIndividual(&nextGenPop[i], amountOfDeclaredParams);
    }
}

void RandomInitialization::randAnIndividual(GAIndividual *memb, int amountOfDeclaredParams) {
    memb->global_fitness = -1; //Reset the global fitness
    for (int j = 0; j < amountOfDeclaredParams; j++) {

        double maxRange = ((maxParamValues[j] - minParamValues[j]) / stepSizeValues[j]);
        double minRange = 0;
        double maxoffset = maxRange - minRange;


        if (maxoffset == 0) {
            memb->sol.param[j] = minParamValues[j];
        }
        else {
            double offset = fmod(rand(), maxoffset + 1);
            memb->sol.param[j] = offset * stepSizeValues[j] + minParamValues[j];
        }
    }
}