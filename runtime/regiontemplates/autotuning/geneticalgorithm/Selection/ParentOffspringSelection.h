//
// Created by taveira on 11/20/15.
//

#ifndef GA_PARENTOFFSPRINGSELECTION_H
#define GA_PARENTOFFSPRINGSELECTION_H

#include "SelectionInterface.h"

class ParentOffspringSelection : public SelectionInterface {
    int amountOfDeclaredParams;

    void selection(GAIndividual *currentGenPop, GAIndividual *nextGenPop, long popsize);

public:
    ParentOffspringSelection(int amountOfDeclaredParams) : amountOfDeclaredParams(amountOfDeclaredParams) { }
};


#endif //GA_PARENTOFFSPRINGSELECTION_H
