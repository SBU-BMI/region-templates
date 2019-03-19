//
// Created by taveira on 4/22/15.
//

#ifndef GA_UTILITIES_H
#define GA_UTILITIES_H

/*OpenMP settings if enabled*/
#ifdef _OPENMP
#include <omp.h>
#endif


#include "opencv2/opencv.hpp"
// #include "opencv2/gpu/gpu.hpp" // old opencv 2.4
#include "opencv2/cudaarithm.hpp" // new opencv 3.4.1

#include <iostream>
#include <stdio.h>
#include <vector>
#include "FileUtils.h"
#include <algorithm>


class SetOfParameters {
public:
    double *param;

    /* Swap pt1 and pt2 */
    static void swapParams(double *pt1, double *pt2);

};


/* A member of our population */
class GAIndividual {
public:
    SetOfParameters sol; // This member's parameter_set (chromosome)
    double global_fitness;

    /* Copy the contents of a tuning member to another*/
    static void copyTuningMemb(GAIndividual *dest, GAIndividual *src, int amountOfDeclaredParams);
};


#endif //GA_UTILITIES_H
