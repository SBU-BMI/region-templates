//
// Created by taveira on 11/10/15.
//

#ifndef GA_JACCARDINDEX_H
#define GA_JACCARDINDEX_H

#include "../Hadoopgis.h"

class JaccardIndex : public Hadoopgis {
protected:
    string scriptName;

    void callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                    std::string referenceMaskFileName);

public:
    JaccardIndex(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *diffPixels);

    void parseOutput(std::string pathToMaskOutputtedByTheScript);

};


#endif //GA_JACCARDINDEX_H
