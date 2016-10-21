//
// Created by taveira on 9/29/15.
//

#ifndef GA_TUNINGGA_H
#define GA_TUNINGGA_H

#include <assert.h>
#include "../TuningInterface.h"
#include "../TuningParamSet.h"
#include "./Utilities/GAUtilities.h"
#include "Mutation/MutationInterface.h"
#include "Selection/SelectionInterface.h"
#include "Mating/MatingInterface.h"
#include "Propagation/PropagationInterface.h"
#include "Initialization/InitializationInterface.h"
#include "Sorting/SortingInterface.h"


class GeneticAlgorithm : public TuningInterface {
private:
    //Atributes
    int numSets;
    /* Population size - It must be an even number and it should be a multiple of the number of threads.*/
    int mutationchance;
    /* Mutation rate, it varies from 0 to 100. 0 means no chance of mutation and 100 means the params will always mutate.*/
    int crossoverrate;
    /* Crossover rate, it varies from 0 to 100. 0 means no chance of crossover and 100 means 100% rate of crossover*/
    int propagateamount;
    /* Amount of individuals from the elite that replaces the worst solutions per generation.*/
    int printiteration;
/* Print status every iterations/generations (0 = Don't output anything, 1 = Every Iteration) */

    int amountOfDeclaredParams;
    std::map<int, std::string> mapParamLabelToVectorId;
    GAIndividual *pop;
    GAIndividual *nextGenPop;

    //std::vector<TuningParam *> *vectorOfParamTypes;
    std::vector<double> minParamValues;
    std::vector<double> maxParamValues;
    std::vector<double> stepSizeValues;

    cv::Mat normalizedInputImage;  //Normalized input image to be used by the other processes.
    cv::Mat maskReference; //The mask reference - set up by the specialist.



//Interfaces
    InitializationInterface *init;
    MatingInterface *mat;
    MutationInterface *mut;
    SelectionInterface *sel;
    PropagationInterface *prop;
    SortingInterface *srt;


    /* Print the each member's score and the gen number, gen is the number of the current
    * generation */
    void printGeneration(GAIndividual *pop, int gen);


public:
    //GeneticAlgorithm();
    GeneticAlgorithm(int maxNumberOfIterations, int popsize, int mutationchance = 30, int crossoverrate = 50,
                     int propagateamount = 0,
                     int printit = 0);

    //GeneticAlgorithm(vector<TuningParam *> *vectorOfParamTypes);


    GeneticAlgorithm(InitializationInterface *init, MatingInterface *mat, MutationInterface *mut,
                     SelectionInterface *sel, PropagationInterface *prop, SortingInterface *srt)
            : init(init), mat(mat), mut(mut), sel(sel), prop(prop), srt(srt) { }

/*
* For a good parallel set-up if you are using OpenMP, the popsize should be a multiple of the number of threads.
*/

/* Print the score and the parameters of a solution. */
    void printaSolution(GAIndividual *pop);

    void setInitializationMethod(InitializationInterface *init) {
        GeneticAlgorithm::init = init;
    }

    void setMatingMethod(MatingInterface *mat) {
        GeneticAlgorithm::mat = mat;
    }

    void setMutationMethod(MutationInterface *mut) {
        GeneticAlgorithm::mut = mut;
    }


    void setSelectionMethod(SelectionInterface *sel) {
        GeneticAlgorithm::sel = sel;
    }

    void setPropagationMethod(PropagationInterface *prop) {
        GeneticAlgorithm::prop = prop;
    }

    void setSortingMethod(SortingInterface *srt) {
        GeneticAlgorithm::srt = srt;
    }

    static void copyPopulation(GAIndividual *dst, GAIndividual *src, int numSets, int amountOfDeclaredParams);

    static void copyIndividual(GAIndividual *dest, GAIndividual *src, int amountOfDeclaredParams);

    bool hasConverged();

    int declareParam(std::string paramLabel, double paramLowerBoundary, double paramHigherBoundary,
                     double paramStepSize);

    int initialize(int argc, char **argv);

    int configure();

    int fetchParams();

    TuningParamSet *getParamSet(int setId = 0);

    double getParamValue(std::string paramName, int setId = 0);

    int reportScore(double scoreValue, int setId);

    void nextIteration();
};

#endif //GA_TUNINGGA_H
