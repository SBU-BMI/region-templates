//
// Created by taveira on 11/20/15.
//

#ifndef GA_NATURALSELECTION_H
#define GA_NATURALSELECTION_H

#include "SelectionInterface.h"
#include "../Sorting/SortingInterface.h"

class NaturalSelection : public SelectionInterface {
private:
    SortingInterface *srt;
    int amountOfDeclaredParams;
public:
    NaturalSelection(SortingInterface *srt, int amountOfDeclaredParams) {
        this->srt = srt;
        this->amountOfDeclaredParams = amountOfDeclaredParams;
    }

    void selection(GAIndividual *currentGenPop, GAIndividual *nextGenPop, long popsize);

};


#endif //GA_NATURALSELECTION_H
