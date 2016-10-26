//
// Created by taveira on 11/20/15.
//

#ifndef GA_SELECTIONINTERFACE_H
#define GA_SELECTIONINTERFACE_H

#include "../Utilities/GAUtilities.h"

class SelectionInterface {
public:
    /* Select the fittest individuals of the population
     * The selection method depends on the fitness function to know what fit means.
     * */
    virtual void selection(GAIndividual *currentGenPop, GAIndividual *nextGenPop, long popsize) = 0;

};

#endif //GA_SELECTIONINTERFACE_H
