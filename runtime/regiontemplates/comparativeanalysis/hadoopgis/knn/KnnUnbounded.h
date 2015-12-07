//
// Created by taveira on 12/4/15.
//

#ifndef RUNTIME_KNNUNBOUNDED_H
#define RUNTIME_KNNUNBOUNDED_H

#include "../Hadoopgis.h"

class KnnUnbounded : public Hadoopgis {
protected:
    string scriptName;
    long k;

    void callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                    std::string referenceMaskFileName);

    void executeScript(std::string pathToScript, std::string scriptName, std::string pathToHadoopgisBuild,
                       std::string maskFileName,
                       std::string referenceMaskFileName);

public:
    KnnUnbounded(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *diffPixels, long k);

    void parseOutput(std::string pathToMaskOutputtedByTheScript);

};

#endif //RUNTIME_KNNUNBOUNDED_H
