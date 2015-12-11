//
// Created by taveira on 12/4/15.
//

#ifndef RUNTIME_KNNUNBOUNDED_H
#define RUNTIME_KNNUNBOUNDED_H

#include "../Hadoopgis.h"
#include "KnnResult.h"

class KnnUnbounded : public Hadoopgis {
protected:
    string scriptName;
    KnnResult *result;
    long k;

    void callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                    std::string referenceMaskFileName);

    void executeScript(std::string pathToScript, std::string scriptName, std::string pathToHadoopgisBuild,
                       std::string maskFileName,
                       std::string referenceMaskFileName);

public:
    KnnUnbounded(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *id, long k);

    KnnUnbounded(std::vector<std::vector<cv::Point> > *list1, std::vector<std::vector<cv::Point> > *list2, float *id,
                 long k);

    void parseOutput(std::string pathToMaskOutputtedByTheScript);

    //KnnResult *getKnnResult();

};

#endif //RUNTIME_KNNUNBOUNDED_H
