//
// Created by taveira on 11/22/15.
//

#ifndef GA_SORTINGINTERFACE_H
#define GA_SORTINGINTERFACE_H

#include "../Utilities/GAUtilities.h"
//#include "../Fitness/FitnessInterface.h"

class SortingInterface {
public:
    /* Population Sorting Interface*/
    virtual void sortPopulation(GAIndividual *pop, long popsize) = 0;

};

#endif //GA_SORTINGINTERFACE_H
