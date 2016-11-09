//
// Created by taveira on 9/29/15.
//

#include <assert.h>
#include "GeneticAlgorithm.h"

#include "Mutation/RandomMutation.h"
#include "Selection/ParentOffspringSelection.h"
#include "Selection/NaturalSelection.h"
#include "Mating/CutSwapMating.h"
#include "Propagation/EliteMembersPropagation.h"
#include "Initialization/RandomInitialization.h"
#include "Sorting/InsertionSorting.h"


GeneticAlgorithm::GeneticAlgorithm(int maxNumberOfIterations, int popsize, int mutationchance, int crossoverrate,
                                   int propagateamount,
                                   int printit) {
    this->maxNumberOfIterations = maxNumberOfIterations;
    this->numSets = popsize;
    this->mutationchance = mutationchance;
    this->crossoverrate = crossoverrate;
    this->propagateamount = propagateamount;
    this->printiteration = printit;

    iteration = 0;
    amountOfDeclaredParams = 0;

    pop = new GAIndividual[numSets];
    nextGenPop = new GAIndividual[numSets];

    this->tuningParamSet = (TuningParamSet **) malloc(numSets * sizeof(TuningParamSet *));
    for (int i = 0; i < numSets; ++i) {
        this->tuningParamSet[i] = new TuningParamSet();
    }

}

int GeneticAlgorithm::initialize(int argc, char **argv) {

    if (this->printiteration > 0) {
        cout << "\tPopulation Size: " << numSets << endl;
        cout << "\tMaximum Number of Generations: " << maxNumberOfIterations << endl;
        cout << "\tElite Size (Propagate Amount): " << propagateamount << endl;
        cout << "\tMutation Rate: " << (mutationchance) << " % " << endl;
    }

    if (fmod(numSets, 2)) {
        cout << "\tThe GA was not configured properly: the population number must be an even number" << endl;
        return -1;
    }
    if ((propagateamount >=
         numSets / 2)) {
        cout << "\tThe GA was not configured properly: the elite size must be smaller than half the population " <<
        endl;
        return -1;
    }
    if ((mutationchance < 0 || mutationchance > 100)) {
        cout << "\tThe GA was not configured properly: the mutation chance must be an INT between 0 and 100" << endl;
        return -1;
    }


    assert(!fmod(numSets, 2)); //Check if pop_size is an even number - Because of mating stage.
    assert ((propagateamount <
             numSets / 2)); //Check if the number of bad solutions are multiple of 2 and bigger than 2.
    assert(mutationchance >= 0 && mutationchance <= 100); //Check if the mutation is a percentage number.

    return 0;
}

int GeneticAlgorithm::declareParam(std::string paramLabel, double paramLowerBoundary, double paramHigherBoundary,
                                   double paramStepSize) {

    //Declare the params for each individual
    for (int i = 0; i < numSets; ++i) {
        double *newParam = (double *) malloc(sizeof(double));
        *(newParam) = paramLowerBoundary;
        tuningParamSet[i]->addParam(paramLabel, newParam);
    }
    mapParamLabelToVectorId[amountOfDeclaredParams] = paramLabel;
    minParamValues.push_back(paramLowerBoundary);
    maxParamValues.push_back(paramHigherBoundary);
    stepSizeValues.push_back(paramStepSize);

    amountOfDeclaredParams++;

    return 0;

}


int GeneticAlgorithm::configure() {


    for (int i = 0; i < numSets; ++i) {
        pop[i].sol.param = new double[amountOfDeclaredParams];
        nextGenPop[i].sol.param = new double[amountOfDeclaredParams];
    }

    /* Default interface implementations */
    init = new RandomInitialization(amountOfDeclaredParams, minParamValues, maxParamValues, stepSizeValues);
    mat = new CutSwapMating(amountOfDeclaredParams);
    mut = new RandomMutation(amountOfDeclaredParams, minParamValues, maxParamValues, stepSizeValues);
    //fit = new DiceCoefficient();
    srt = new InsertionSorting(amountOfDeclaredParams);
    sel = new NaturalSelection(srt, amountOfDeclaredParams);
    prop = new EliteMembersPropagation(amountOfDeclaredParams);

    init->initializePopulationAttributes(pop, nextGenPop, numSets);

    return 0;
}

bool GeneticAlgorithm::hasConverged() {
    return iteration >= maxNumberOfIterations;
}


int GeneticAlgorithm::fetchParams() {
#ifdef _OPENMP
#pragma omp parallel
    {
#endif
    mat->matePopulation(pop, nextGenPop,
                        numSets, crossoverrate); // Mate the population (pop). Generates nextGenPop.
    mut->mutate(nextGenPop, numSets, mutationchance); //Mutate nextGenPop.

    //Update the paramMap
#pragma omp for schedule (dynamic)
    for (int setId = 0; setId < numSets; ++setId) {
        for (int paramId = 0; paramId < amountOfDeclaredParams; ++paramId) {
            std::string paramLabel = mapParamLabelToVectorId[paramId];
            tuningParamSet[setId]->updateParamValue(paramLabel, nextGenPop[setId].sol.param[paramId]);
        }
    }
#ifdef _OPENMP
    } //end parallel section
#endif

    return 0;
}

TuningParamSet *GeneticAlgorithm::getParamSet(int setId) {
    return tuningParamSet[setId];
}

double GeneticAlgorithm::getParamValue(std::string paramName, int setId) {
    return *(tuningParamSet[setId]->paramSet.find(paramName)->second);
}

int GeneticAlgorithm::reportScore(double scoreValue, int setId) {
    tuningParamSet[setId]->setScore(scoreValue);
    nextGenPop[setId].global_fitness = scoreValue;
    return 0;
}

void GeneticAlgorithm::copyPopulation(GAIndividual *dest, GAIndividual *src, int numSets, int amountOfDeclaredParams) {
    for (int i = 0; i < numSets; ++i) {
        copyIndividual(&dest[i], &src[i], amountOfDeclaredParams);
    }
}

void GeneticAlgorithm::copyIndividual(GAIndividual *dest, GAIndividual *src, int amountOfDeclaredParams) {
    // Copy the fitness
    dest->global_fitness = src->global_fitness;
    // Copy the parameters
    for (int i = 0; i < amountOfDeclaredParams; ++i) {
        dest->sol.param[i] = src->sol.param[i];
    }
}

void GeneticAlgorithm::nextIteration() {
    if (iteration == 0) {
        copyPopulation(pop, nextGenPop, numSets, amountOfDeclaredParams);
    }
#ifdef _OPENMP
#pragma omp parallel
    {
#endif
    sel->selection(pop, nextGenPop,
                   numSets); //Select the individuals from pop and nextGenPop that will survive.
    srt->sortPopulation(pop, numSets); //Sort the population
    printGeneration(pop, iteration);
    prop->propagateMembers(pop, numSets,
                           propagateamount);
#ifdef _OPENMP
    } //end parallel section
#endif

    iteration++;
}

void GeneticAlgorithm::printGeneration(GAIndividual *pop, int gen) {
#ifdef _OPENMP
#pragma omp single
    {
#endif
    /* Print stats every printiteration iterations of the loop */
    if (printiteration > 0 && gen % printiteration == 0) {
        int i;

        printf("At gen %d, the population fitness is:", gen);
        for (i = 0; i < numSets; i++)
            printf("  (%.6lf)", ((pop[i].global_fitness)));
        printf("\n");

    }


#ifdef _OPENMP
    } //end omp single
#endif
}

void GeneticAlgorithm::printaSolution(GAIndividual *pop) {
    for (int i = 0; i < amountOfDeclaredParams; i++)
        printf("  [%.2lf]\t", ((pop->sol.param[i])));
    printf(" --- ");
    printf(" Global = {%.6lf}\t", ((pop->global_fitness)));
    printf("\n");
}
