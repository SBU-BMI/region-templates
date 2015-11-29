//
// Created by taveira on 11/10/15.
//

#include "DiceCoefficient.h"

DiceCoefficient::DiceCoefficient(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *diffPixels) {
    this->dr1 = dr1;
    this->dr2 = dr2;
    this->diff = diffPixels;
    this->scriptName = "DiceCoefficient.sh";
}

void DiceCoefficient::parseOutput(std::string myMaskPath) {


    myMaskPath.append(outputFileExtension);
    std::ifstream infile(myMaskPath.c_str());
    std::string line;
    float area;
    float totalArea = 0;
    int numberOfPolygonsIntersections = 0;
    //Get the total area of the intersection
    while (std::getline(infile, line)) {
        infile >> area;
        totalArea += area;
        numberOfPolygonsIntersections++;
    }

    if (remove(myMaskPath.c_str()) != 0)
        perror("Comparative Analysis - HadoopGIS - Dice: Error deleting temporary file. Are you executing simultaneosly the same program?\n");

    float compId = diff[0];
    this->diff[0] = totalArea;
    this->diff[1] = numberOfPolygonsIntersections;
    std::cout << "CompId: " << compId << " Dice Coefficient: " << this->diff[0] <<
    " Number of Polygons Intersections: " << this->diff[1] <<
    std::endl;


}

void  DiceCoefficient::callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                                  std::string referenceMaskFileName) {
    executeScript(pathToScript, this->scriptName, pathToHadoopgisBuild, maskFileName, referenceMaskFileName);

}