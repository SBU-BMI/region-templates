//
// Created by taveira on 11/22/15.
//

#include "InsertionSorting.h"

void InsertionSorting::sortPopulation(GAIndividual *pop, long popsize) {
#ifdef _OPENMP
#pragma omp single
    {
#endif
    int i, j;
    long k;
    GAIndividual temp;
    temp.sol.param = new double[amountOfDeclaredParams];
    k = popsize;

    //Increasing Order
    for (i = 0; i < k; i++) {
        //temp = pop[i];
        GAIndividual::copyTuningMemb(&temp, &pop[i], amountOfDeclaredParams);
        j = i - 1;
        while (j >= 0 && pop[j].global_fitness > temp.global_fitness) {
            //pop[j + 1] = pop[j];
            GAIndividual::copyTuningMemb(&pop[j + 1], &pop[j], amountOfDeclaredParams);
            j = j - 1;
        }
        //pop[j + 1] = temp;
        GAIndividual::copyTuningMemb(&pop[j + 1], &temp, amountOfDeclaredParams);

    }

#ifdef _OPENMP
    } //end omp single
#endif

};