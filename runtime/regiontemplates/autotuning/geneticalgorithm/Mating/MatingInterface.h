//
// Created by taveira on 11/22/15.
//

#ifndef GA_MATINGINTERFACE_H
#define GA_MATINGINTERFACE_H

#include "../Utilities/GAUtilities.h"

class MatingInterface {
public:
    /* Mate individuals of a population generating a nextGen population*/
    virtual void matePopulation(GAIndividual *pop,
                                GAIndividual *nextGenPop, int popsize, int crossoverrate) = 0;
};

#endif //GA_MATINGINTERFACE_H
