//
// Created by taveira on 11/10/15.
//

#include "DiceCoefficient.h"

DiceCoefficient::DiceCoefficient(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *diffPixels) {
    this->dr1 = dr1;
    this->dr2 = dr2;
    getPolygonsFromLabeledMask(this->dr1->getData(), this->listOfPolygons[0]);
    getPolygonsFromLabeledMask(this->dr2->getData(), this->listOfPolygons[1]);
    this->diff = diffPixels;
    this->scriptName = "SpatialQuery.sh";
}

void DiceCoefficient::parseOutput(std::string myMaskPath, double a1, double a2) {


    myMaskPath.append(outputFileExtension);
    std::ifstream infile(myMaskPath.c_str());
    std::string line;
    float intersectArea;
    float totalIntersectArea = 0;
    //Get the total area of the intersection
    while (std::getline(infile, line)) {
        stringstream ss(line);
        ss >> intersectArea;
        totalIntersectArea += intersectArea;
    }

    if (remove(myMaskPath.c_str()) != 0)
        perror("Comparative Analysis - HadoopGIS - Dice: Error deleting temporary file. Are you executing simultaneosly the same program?\n");

    float totalArea = (float) (a1 + a2);
    if (totalIntersectArea <= 0) totalIntersectArea = 0;
    float dice = 2 * totalIntersectArea / (totalArea);
    float compId = diff[0];
    this->diff[0] = dice;
    this->diff[1] = totalArea;
    std::cout << "Comparative Analysis - HadoopGIS - Dice: CompId: " << compId << " Dice Coefficient: " <<
    this->diff[0] <<
    " Intersect Area: " << totalIntersectArea << " Total sets area:" << this->diff[1] <<
    std::endl;


}

void  DiceCoefficient::callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                                  std::string referenceMaskFileName) {
    executeScript(pathToScript, this->scriptName, pathToHadoopgisBuild, maskFileName, referenceMaskFileName);

}