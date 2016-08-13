//
// Created by taveira on 8/8/16.
//

#ifndef RUNTIME_TUNINGPARAMSET_H
#define RUNTIME_TUNINGPARAMSET_H

#include "Util.h"
#include <map>


class TuningParamSet {
protected:
    std::map<std::string, void *> paramSet;
    double score;
public:
    void bindParam(std::string paramName, void *paramValue);
};

void TuningParamSet::bindParam(std::string paramName, void *paramValue) {
    paramSet.insert(paramName, paramValue);
}

void TuningParamSet::setScore(double score) {
    this.score = score;
}

double TuningParamSet::getScore() {
    return this.score;
}

#endif //RUNTIME_TUNINGPARAMSET_H
