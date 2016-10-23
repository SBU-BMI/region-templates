//
// Created by taveira on 11/20/15.
//

#include "NaturalSelection.h"

/* Choose the most fit individuals among the two populations and wipe the rest
 * This function was not implemented to be executed in parallel.*/
void NaturalSelection::selection(GAIndividual *pop, GAIndividual *nextGenPop, long popsize) {

    this->srt->sortPopulation(pop, popsize);
    this->srt->sortPopulation(nextGenPop, popsize);
#ifdef _OPENMP
#pragma omp single
    {
#endif
    //GAIndividual* temp = new GAIndividual[popsize];
    std::vector<GAIndividual> temp(popsize);
    for (int n = 0; n < popsize; ++n) {
        temp[n].sol.param = new double[amountOfDeclaredParams];
    }

    int k = 0;
    int i = 0;
    int j = 0;
    while (k < popsize) {
        //Low is good fitness function
        if (pop[i].global_fitness < nextGenPop[j].global_fitness) {
            GAIndividual::copyTuningMemb(&temp[k], &pop[i], amountOfDeclaredParams);
            k++;
            i++;
        }
        else {
            GAIndividual::copyTuningMemb(&temp[k], &nextGenPop[j], amountOfDeclaredParams);
            k++;
            j++;
        }
    }

    for (int l = 0; l < popsize; ++l) {
        GAIndividual::copyTuningMemb(&pop[l], &temp[l], amountOfDeclaredParams);
    }
    //Deallocate vector memory. Replace with an empty vector.
    vector<GAIndividual>().swap(temp);
#ifdef _OPENMP
    }
#endif

}