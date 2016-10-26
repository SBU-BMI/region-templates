//
// Created by taveira on 11/19/15.
//

#include "RandomMutation.h"

// Mutates randomly some part of the population
void RandomMutation::mutate(GAIndividual *nextGenPop, long popsize,
                            long mutationchnc) {
#ifdef _OPENMP
#pragma omp for schedule (dynamic)
#endif
    for (int i = 0; i < popsize; i++) {
        for (int j = 0; j < amountOfDeclaredParams; ++j) {
            if (rand() % 101 < mutationchnc) {
                int randomParam = j;
                double maxoffset = floor((maxParamValues[randomParam] - minParamValues[randomParam]) /
                                         stepSizeValues[randomParam]);

                if (maxoffset == 0) {
                    nextGenPop[i].sol.param[randomParam] = minParamValues[randomParam];
                }
                else {
                    double offset = fmod(rand(), maxoffset + 1);
                    nextGenPop[i].sol.param[randomParam] =
                            offset * stepSizeValues[randomParam] + minParamValues[randomParam];
                }
            }
        }

    }
}