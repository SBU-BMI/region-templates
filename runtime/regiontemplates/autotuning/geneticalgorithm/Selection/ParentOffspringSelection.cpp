//
// Created by taveira on 11/20/15.
//

#include "ParentOffspringSelection.h"

/* Choose the most fit (higher fitness value) individuals among a parent and their respective offspring and wipe the other one */
void ParentOffspringSelection::selection(GAIndividual *pop, GAIndividual *nextGenPop, long popsize) {
#ifdef _OPENMP
#pragma omp for schedule (dynamic)
#endif
    for (int i = 0; i < popsize; ++i) {
        if (pop[i].global_fitness > nextGenPop[i].global_fitness) {
            //Replace parent with offspring.
            GAIndividual::copyTuningMemb(&pop[i], &nextGenPop[i], amountOfDeclaredParams);

        }

    }

}