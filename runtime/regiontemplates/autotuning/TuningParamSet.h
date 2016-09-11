//
// Created by taveira on 8/8/16.
//

#ifndef RUNTIME_TUNINGPARAMSET_H
#define RUNTIME_TUNINGPARAMSET_H

#include "Util.h"
#include <map>


class TuningParamSet {
public:
    std::map<std::string, void *> paramSet;
    double score;

    void addParam(std::string paramName, void *paramInitialValue);

    void setScore(double score);

    double getScore();

};

void TuningParamSet::addParam(std::string paramName, void *paramInitialValue) {
    //paramSet.insert(paramName, paramValue);
    paramSet[paramName] = paramInitialValue;
}

void TuningParamSet::setScore(double score) {
    this->score = score;
}

double TuningParamSet::getScore() {
    return this->score;
}

#endif //RUNTIME_TUNINGPARAMSET_H
