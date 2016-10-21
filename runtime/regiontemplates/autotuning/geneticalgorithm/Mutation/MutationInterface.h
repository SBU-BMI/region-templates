//
// Created by taveira on 11/19/15.
//


#ifndef GA_MUTATIONINTERFACE_H
#define GA_MUTATIONINTERFACE_H

#include "../Utilities/GAUtilities.h"

class MutationInterface {
public:
    /* Mutate some individuals of the population*/
    virtual void mutate(GAIndividual *nextGenPop, long popsize, long mutationchnc) = 0;
};

#endif //GA_MUTATIONINTERFACE_H
