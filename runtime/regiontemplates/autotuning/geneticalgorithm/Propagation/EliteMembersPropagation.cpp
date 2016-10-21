//
// Created by taveira on 11/22/15.
//

#include "EliteMembersPropagation.h"

/* Replace the worst solutions by a copy of the elite members params.
 */
void EliteMembersPropagation::propagateMembers(GAIndividual *pop, long popsize, long propagateamount) {
#ifdef _OPENMP
#pragma omp for schedule (dynamic)
#endif
    for (int i = 0; i < propagateamount; i++) {
        GAIndividual::copyTuningMemb(&pop[popsize - i - 1], &pop[i], amountOfDeclaredParams);
    }
}