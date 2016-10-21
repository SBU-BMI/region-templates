//
// Created by taveira on 11/22/15.
//

#ifndef GA_PROPAGATIONINTERFACE_H
#define GA_PROPAGATIONINTERFACE_H

#include "../Utilities/GAUtilities.h"

class PropagationInterface {
public:
    /* Replace the worst individuals by a copy of the elite params. */
    virtual void propagateMembers(GAIndividual *pop, long popsize, long propagateamount) = 0;
};

#endif //GA_PROPAGATIONINTERFACE_H
