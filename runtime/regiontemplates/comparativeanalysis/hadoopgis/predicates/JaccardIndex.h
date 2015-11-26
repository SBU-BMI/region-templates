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
    JaccardIndex() {
        this->scriptName = "JaccardIndex.sh";
    }

    double parseOutput(std::string pathToMaskOutputtedByTheScript);

    const int getFitnessType() { return HIGH_IS_GOOD_TYPE; };

};


#endif //GA_JACCARDINDEX_H
