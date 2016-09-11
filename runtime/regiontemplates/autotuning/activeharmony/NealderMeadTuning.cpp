//
// Created by taveira on 8/31/16.
//

#include <stdio.h>
#include <unistd.h>
#include "NealderMeadTuning.h"

NealderMeadTuning::NealderMeadTuning(int argc, char **argv) {
    iteration = -1;
    harmonySessionStatus = initialize(argc, argv);
}


int NealderMeadTuning::initialize(int argc, char **argv) {

    // AH SETUP //
    /* Initialize a Harmony client. */
    for (int i = 0; i < numClients; i++) {
        hdesc.push_back(harmony_init(&argc, &argv));
        if (hdesc[i] == NULL) {
            fprintf(stderr, "Failed to initialize a harmony session.\n");
            return -1;
        }
    }

    snprintf(name, sizeof(name), "NealderMeadTuning.%d", getpid());


    if (harmony_session_name(hdesc[0], name) != 0) {
        fprintf(stderr, "Could not set session name.\n");
        return -1;
    }
    else return 0;

}

int NealderMeadTuning::declareParam(std::string paramLabel, void *paramLowerBoundary, void *paramHigherBoundary,
                                    void *paramStepSize, int setId) {
    //CHANCES DE BUG.
    if (harmony_real(hdesc[0], paramLabel.c_str(), *((double *) paramLowerBoundary), *((double *) paramHigherBoundary),
                     *((double *) paramStepSize)) != 0) {
        fprintf(stderr, "Failed to define tuning session\n");
        return -1;
    }
    else {
        void *newParam = (double *) malloc(sizeof(double));
        *((double *) newParam) = *((double *) paramLowerBoundary);
        tuningParamSet->addParam(paramLabel, ((double *) paramLowerBoundary));
        return 0;
    }
}

int NealderMeadTuning::configure(int strategy, int maxNumberOfIterations) {

    this->maxNumberOfIterations = maxNumberOfIterations;

    std::string AHpolicy;
    switch (strategy) {
        case 0:
            harmony_strategy(hdesc[0], "nm.so");
            AHpolicy.append("nm.so");
            break;
        default:
            harmony_strategy(hdesc[0], "pro.so");
            AHpolicy.append("pro.so");
            break;
    }
    std::cout << "AH configuration: " << AHpolicy << std::endl;
//    if(initPercent.size()>0 ){
//        harmony_setcfg(hdesc[0], "INIT_PERCENT", initPercent.c_str());
//        std::cout << "AH configuration: "<< AHpolicy << " INIT_PERCENT: "<< initPercent << std::endl;
//    }
    char numbuf[12];
    snprintf(numbuf, sizeof(numbuf), "%d", numClients);

    harmony_setcfg(hdesc[0], "CLIENT_COUNT", numbuf);

    printf("Starting Harmony...\n");
    if (harmony_launch(hdesc[0], NULL, 0) != 0) {
        fprintf(stderr,
                "Could not launch tuning session: %s. E.g. export HARMONY_HOME=$HOME/region-templates/runtime/build/regiontemplates/external-src/activeharmony-4.5/\n",
                harmony_error_string(hdesc[0]));

        return -1;
    }


    //bind params

    typedef std::map<std::string, void *>::iterator it_type;
    for (it_type iterator = tuningParamSet[0].paramSet.begin();
         iterator != tuningParamSet[0].paramSet.end(); iterator++) {
        //iterator.first key
        //iterator.second value
        if (bindParam(iterator->first, 0) != 0) {
            fprintf(stderr, "Failed to register variable %s\n", iterator->first.c_str());
            harmony_fini(hdesc[0]);
            return -1;
        }
    }


    printf("Bind %d successful\n", 0);

    /* Join this client to the tuning session we established above. */
    for (int i = 0; i < numClients; i++) {
        if (harmony_join(hdesc[i], NULL, 0, name) != 0) {
            fprintf(stderr, "Could not connect to harmony server: %s\n",
                    harmony_error_string(hdesc[i]));
            harmony_fini(hdesc[i]);
            return -1;
        }
    }

    return 0;

}

int NealderMeadTuning::bindParam(std::string paramLabel, int setId) {


    //CHANCES DE BUG
    if (harmony_bind_real(hdesc[0], paramLabel.c_str(), ((double *) tuningParamSet[setId].paramSet[paramLabel])) != 0) {
        fprintf(stderr, "Failed to register variable\n");
        return -1;
    }
    else return 0;
}


int NealderMeadTuning::nextIteration() {

    iteration++;

    for (int i = 0; i < numClients; i++) {
        int hresult;
        do {
            hresult = harmony_fetch(hdesc[i]);
        } while (hresult == 0);
    }

}


TuningParamSet *NealderMeadTuning::fetchParamSet(int setId) {
    return &tuningParamSet[setId];
}

int NealderMeadTuning::reportScore(void *scoreValue, int setId) {
    tuningParamSet[setId].score = *((double *) scoreValue);
    // Report the performance we've just measured.
    if (harmony_report(hdesc[setId], *((double *) scoreValue)) != 0) {
        fprintf(stderr, "Failed to report performance to server.\n");
        harmony_fini(hdesc[setId]);
        return -1;
    }
}

bool NealderMeadTuning::hasConverged() {
    return iteration >= maxNumberOfIterations;
}
