//
// Created by taveira on 11/8/15.
//

#ifndef GA_HADOOPGIS_H
#define GA_HADOOPGIS_H

#include <iostream>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include "../TaskDiffMask.h"

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#define HADOOPGIS_SCRIPTS_DIR HADOOPGIS_SCRIPTS_DIRECTORY
#define WRITE_ENABLED_TEMP_PATH WRITE_ENABLED_TEMPORARY_PATH
#define HADOOPGIS_BUILD_DIR HADOOPGIS_BUILD_DIRECTORY

class Hadoopgis : public TaskDiffMask {

protected:
    float amountOfPolygons;
    float totalAreaOfPolygons;

    std::vector<std::vector<cv::Point> > *listOfPolygons[2];

    virtual void parseOutput(std::string pathToMaskOutputtedByTheScript) = 0;

    virtual void callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                            std::string referenceMaskFileName) = 0;

    bool compare_points(const cv::Point &e1, const cv::Point &e2);

    virtual void executeScript(std::string pathToScript, std::string scriptName, std::string pathToHadoopgisBuild,
                               std::string maskFileName,
                               std::string referenceMaskFileName);

    void convertPolygonToHadoopgisInput(std::vector<std::vector<cv::Point> > *hull, std::ofstream &ss);

public:
    static void getPolygonsFromMask(const cv::Mat &img, std::vector<std::vector<cv::Point> > *&listOfPolygons);

    bool run(int procType = ExecEngineConstants::CPU, int tid = 0);
};

const string space = " ";
//Script output file extension
const string outputFileExtension = ".output";
#endif //GA_HADOOPGIS_H
