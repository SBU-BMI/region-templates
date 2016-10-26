//
// Created by taveira on 11/19/15.
//


#ifndef GA_RANDOMMUTATION_H
#define GA_RANDOMMUTATION_H

#include "MutationInterface.h"

// Mutates randomly some part of the population
class RandomMutation : public MutationInterface {
    int amountOfDeclaredParams;
    std::vector<double> minParamValues;
    std::vector<double> maxParamValues;
    std::vector<double> stepSizeValues;
public:
    void mutate(GAIndividual *nextGenPop, long popsize,
                long mutationchnc);

    RandomMutation(int amountOfDeclaredParams, std::vector<double> &minParamValues, std::vector<double> &maxParamValues,
                   std::vector<double> &stepSizeValues) {
        this->amountOfDeclaredParams = amountOfDeclaredParams;
        this->minParamValues = minParamValues;
        this->maxParamValues = maxParamValues;
        this->stepSizeValues = stepSizeValues;
        /* Find the other solutions using the TUNNING.*/
        srand((unsigned int) time(NULL)); /* Seed rand() with the time */
    }


};

#endif //GA_RANDOMMUTATION_H
