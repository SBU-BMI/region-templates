//
// Created by taveira on 8/8/16.
//

#ifndef RUNTIME_TUNINGPARAMSET_H
#define RUNTIME_TUNINGPARAMSET_H

#include "Util.h"
#include <map>
#include <string>
#include <iostream>

class TuningParamSet {
public:
    std::map<std::string, double *> paramSet;
    double score;


    void addParam(std::string paramName, double *paramInitialValue) {
        //paramSet.insert(paramName, paramValue);
        paramSet[paramName] = paramInitialValue;
    }

    void setScore(double score) {
        this->score = score;
    }

    double getScore() {
        return this->score;
    }

    void updateParamValue(std::string paramName, double newValue) {
        //double * referenceValue = paramSet[paramName];

        double *referenceValue = (paramSet.find(paramName)->second);
        *referenceValue = newValue;
    }
};


#endif //RUNTIME_TUNINGPARAMSET_H
