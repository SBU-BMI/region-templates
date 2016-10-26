//
// Created by taveira on 11/22/15.
//

#ifndef GA_ELITEMEMBERSPROPAGATION_H
#define GA_ELITEMEMBERSPROPAGATION_H


#include "PropagationInterface.h"

class EliteMembersPropagation : public PropagationInterface {
    int amountOfDeclaredParams;

    void propagateMembers(GAIndividual *pop, long popsize, long propagateamount);


public:
    EliteMembersPropagation(int amountOfDeclaredParams) : amountOfDeclaredParams(amountOfDeclaredParams) { }
};


#endif //GA_ELITEMEMBERSPROPAGATION_H
