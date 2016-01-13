//
// Created by taveira on 11/10/15.
//

#ifndef GA_DICECOEFFICIENT_H
#define GA_DICECOEFFICIENT_H

#include "../Hadoopgis.h"

class DiceCoefficient : public Hadoopgis {
protected:
    string scriptName;

    void callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                    std::string referenceMaskFileName);

public:
    DiceCoefficient(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *diffPixels);

    void parseOutput(std::string pathToMaskOutputtedByTheScript, double area1, double area2);

};

#endif //GA_DICECOEFFICIENT_H
