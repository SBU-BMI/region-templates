//
// Created by taveira on 11/9/15.
//

#include "MaskIntersection.h"


double MaskIntersection::parseOutput(std::string myMaskPath) {


    myMaskPath.append(outputFileExtension);
    std::ifstream infile(myMaskPath.c_str());
    std::string line;
    double area;
    double totalArea = 0;
    //Get the total area of the intersection
    while (std::getline(infile, line)) {
        infile >> area;
        totalArea += area;
    }
//TODO use logger to report error
//    if (remove(myMaskPath.c_str()) != 0)
//        perror("Error deleting file");
    return totalArea;
}

void  MaskIntersection::callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                                   std::string referenceMaskFileName) {
    executeScript(pathToScript, this->scriptName, pathToHadoopgisBuild, maskFileName, referenceMaskFileName);

}
