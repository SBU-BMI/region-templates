//
// Created by taveira on 11/22/15.
//

#ifndef GA_CUTSWAPMATING_H
#define GA_CUTSWAPMATING_H


#include "MatingInterface.h"

class CutSwapMating : public MatingInterface {
    int amountOfDeclaredParams;

    void matePopulation(GAIndividual *pop, GAIndividual *nextGenPop,
                        int popsize, int crossoverrate);


public:
    CutSwapMating(int amountOfDeclaredParams) {
        this->amountOfDeclaredParams = amountOfDeclaredParams;
    }
};


#endif //GA_CUTSWAPMATING_H
