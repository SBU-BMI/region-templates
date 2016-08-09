//
// Created by taveira on 7/31/16.
//

#ifndef RUNTIME_DICENOTCOOLCOEFFICIENT_H
#define RUNTIME_DICENOTCOOLCOEFFICIENT_H

#include "../Hadoopgis.h"

class DiceNotCoolCoefficient : public Hadoopgis {
protected:
    string scriptName;

    void callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                    std::string referenceMaskFileName);

public:
    DiceNotCoolCoefficient(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *diffPixels);

    void parseOutput(std::string pathToMaskOutputtedByTheScript, double area1, double area2);

};


#endif //RUNTIME_DICENOTCOOLCOEFFICIENT_H
