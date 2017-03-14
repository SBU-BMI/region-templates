//
// Created by taveira on 12/4/15.
//

#ifndef RUNTIME_KNNUNBOUNDED_H
#define RUNTIME_KNNUNBOUNDED_H

#include "../Hadoopgis.h"
#include "KnnResult.h"

class KnnBounded : public Hadoopgis {
protected:
    string scriptName;
    KnnResult *result;
    long k;
    float boundary;

    void callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                    std::string referenceMaskFileName);

    void executeScript(std::string pathToScript, std::string scriptName, std::string pathToHadoopgisBuild,
                       std::string maskFileName,
                       std::string referenceMaskFileName);

public:
    KnnBounded(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *id, long k, float boundary);

    KnnBounded(std::vector<std::vector<std::vector<cv::Point> > *> *list1,
               std::vector<std::vector<std::vector<cv::Point> > *> *list2, float *id,
               long k, float boundary);

    void parseOutput(std::string pathToMaskOutputtedByTheScript, double area1, double area2);

    //KnnResult *getKnnResult();

};

#endif //RUNTIME_KNNUNBOUNDED_H
