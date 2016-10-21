//
// Created by taveira on 11/22/15.
//

#ifndef GA_INITIALIZATIONINTERFACE_H
#define GA_INITIALIZATIONINTERFACE_H

#include "../Utilities/GAUtilities.h"

class InitializationInterface {

public:
    /* Initialize population method */
    virtual void initializePopulationAttributes(GAIndividual *pop,
                                                GAIndividual *nextGenPop, int popsize) = 0;
};

#endif //GA_INITIALIZATIONINTERFACE_H
