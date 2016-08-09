//
// Created by taveira on 7/31/16.
//

#include "DiceNotCoolCoefficient.h"


DiceNotCoolCoefficient::DiceNotCoolCoefficient(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *diffPixels) {
    this->dr1 = dr1;
    this->dr2 = dr2;
    getPolygonsFromMask(this->dr1->getData(), this->listOfPolygons[0]);
    getPolygonsFromMask(this->dr2->getData(), this->listOfPolygons[1]);
    this->diff = diffPixels;
    this->scriptName = "Dice2Query.sh";
}

void DiceNotCoolCoefficient::parseOutput(std::string myMaskPath, double a1, double a2) {


    myMaskPath.append(outputFileExtension);
    std::ifstream infile(myMaskPath.c_str());
    std::string line;
    float intersectArea;
    float totalDice = 0;
    int count = 0;
    //Get the total area of the intersection
    while (std::getline(infile, line)) {
        stringstream ss(line);
        ss >> intersectArea;
        totalDice += intersectArea;
        count++;
    }

    if (remove(myMaskPath.c_str()) != 0)
        perror("Comparative Analysis - HadoopGIS - Dice 2: Error deleting temporary file. Are you executing simultaneosly the same program?\n");

    float dice = totalDice / count;
    float compId = diff[0];
    this->diff[0] = dice;
    this->diff[1] = count;
    std::cout << "Comparative Analysis - HadoopGIS - Dice 2: CompId: " << compId << " Dice Not Cool Coefficient: " <<
    this->diff[0] <<
    " Total Dice2: " << totalDice << " Amount of intersections:" << this->diff[1] <<
    std::endl;


}

void  DiceNotCoolCoefficient::callScript(std::string pathToScript, std::string pathToHadoopgisBuild,
                                         std::string maskFileName,
                                         std::string referenceMaskFileName) {
    executeScript(pathToScript, this->scriptName, pathToHadoopgisBuild, maskFileName, referenceMaskFileName);

}