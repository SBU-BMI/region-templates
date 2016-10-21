//
// Created by taveira on 11/22/15.
//

#include "CutSwapMating.h"

/* The mating method is a simple cut and swap of parameters:
 *
 * Consider two solutions:
 *
 * parent1: |====================|
 * parent2: |XXXXXXXXXXXXXXXXXXXX|
 *
 * Pick an arbitrary point to cut at:
 *
 * parent1: |==============|======|
 * parent2: |XXXXXXXXXXXXXX|XXXXXX|
 *
 * Swap the part of the solution after that point to form the children:
 *
 * child1:  |==============XXXXXXX|
 * child2:  |XXXXXXXXXXXXXX=======|
 *
 * Reprocess the image.
 *
 *
 * The other members are reproduced and mutated, note that only the children go into the new population; all the parents
 * die.
 */
void CutSwapMating::matePopulation(GAIndividual *pop,
                                   GAIndividual *nextGenPop, int popsize, int crossoverrate) {
#ifdef _OPENMP
#pragma omp for schedule (dynamic)
#endif
    for (int i = 0; i < popsize; i += 2) {
        int randint = rand() % 101;
        if (randint % 101 < crossoverrate) {
            int crossoverPoint = randint % amountOfDeclaredParams;
            nextGenPop[i].global_fitness = -1;
            nextGenPop[i + 1].global_fitness = -1;

            // Create the offspring
            for (int k = 0; k < amountOfDeclaredParams; ++k) {
                nextGenPop[i].sol.param[k] = pop[i].sol.param[k];
                nextGenPop[i + 1].sol.param[k] = pop[i + 1].sol.param[k];

            }

            // The first half of the chromosomes don't change
            // The second half of the chromosomes are swapped
            for (int j = crossoverPoint; j < amountOfDeclaredParams; j++) {
                SetOfParameters::swapParams(&nextGenPop[i].sol.param[j], &nextGenPop[i + 1].sol.param[j]);
            }
        }

    }
}