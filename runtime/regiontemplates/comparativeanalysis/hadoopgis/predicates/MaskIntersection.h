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
    MaskIntersection(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *diffPixels);

    void parseOutput(std::string pathToMaskOutputtedByTheScript, double area1, double area2);

};


#endif //GA_MASKINTERSECTION_H
