//
// Created by taveira on 11/22/15.
//

#ifndef GA_INSERTIONSORT_H
#define GA_INSERTIONSORT_H


#include "SortingInterface.h"

class InsertionSorting : public SortingInterface {
    int amountOfDeclaredParams;
public:
    InsertionSorting(int amountOfDeclaredParams) { this->amountOfDeclaredParams = amountOfDeclaredParams; }

private:
    void sortPopulation(GAIndividual *pop, long popsize);
};


#endif //GA_INSERTIONSORT_H
