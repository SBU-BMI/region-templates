//
// Created by taveira on 11/9/15.
//

#ifndef GA_MASKINTERSECTION_H
#define GA_MASKINTERSECTION_H

#include "../Hadoopgis.h"

class MaskIntersection : public Hadoopgis {
protected:
    string scriptName;

    void callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                    std::string referenceMaskFileName);

public:
    MaskIntersection() {
        scriptName = "MaskIntersection.sh";
    }

    double parseOutput(std::string pathToMaskOutputtedByTheScript);

    const int getFitnessType() { return HIGH_IS_GOOD_TYPE; };
};


#endif //GA_MASKINTERSECTION_H
