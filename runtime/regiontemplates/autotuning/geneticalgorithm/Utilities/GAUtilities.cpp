//
// Created by taveira on 4/22/15.
//

#include "GAUtilities.h"

// Swap pt1 and pt2
void SetOfParameters::swapParams(double *pt1, double *pt2) {
    double pt_tmp;
    pt_tmp = *pt2;
    *pt2 = *pt1;
    *pt1 = pt_tmp;
}

void GAIndividual::copyTuningMemb(GAIndividual *dest, GAIndividual *src, int amountOfDeclaredParams) {
    // Copy the fitness
    dest->global_fitness = src->global_fitness;
    // Copy the parameters
    for (int i = 0; i < amountOfDeclaredParams; ++i) {
        dest->sol.param[i] = src->sol.param[i];
    }

}




