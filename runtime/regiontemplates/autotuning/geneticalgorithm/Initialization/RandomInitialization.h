//
// Created by taveira on 11/22/15.
//

#ifndef GA_RANDOMINITIALIZATION_H
#define GA_RANDOMINITIALIZATION_H


#include "InitializationInterface.h"

class RandomInitialization : public InitializationInterface {
protected:
    int amountOfDeclaredParams;
    std::vector<double> minParamValues;
    std::vector<double> maxParamValues;
    std::vector<double> stepSizeValues;

    /* Make a random individual */
    void randAnIndividual(GAIndividual *pop, int amountOfDeclaredParams);

public:
    void initializePopulationAttributes(GAIndividual *pop,
                                        GAIndividual *nextGenPop, int popsize);

    RandomInitialization(int amountOfDeclaredParams, std::vector<double> &minParamValues,
                         std::vector<double> &maxParamValues, std::vector<double> &stepSizeValues) {
        this->amountOfDeclaredParams = amountOfDeclaredParams;
        this->minParamValues = minParamValues;
        this->maxParamValues = maxParamValues;
        this->stepSizeValues = stepSizeValues;
    }
};


#endif //GA_RANDOMINITIALIZATION_H
