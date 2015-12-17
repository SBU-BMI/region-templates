//
// Created by taveira on 11/10/15.
//

#include "JaccardIndex.h"

JaccardIndex::JaccardIndex(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *diffPixels) {
    this->dr1 = dr1;
    this->dr2 = dr2;
    amountOfPolygons = 0;
    totalAreaOfPolygons = 0;
    getPolygonsFromMask(this->dr1->getData(), this->listOfPolygons[0]);
    getPolygonsFromMask(this->dr2->getData(), this->listOfPolygons[1]);
    this->diff = diffPixels;
    this->scriptName = "JaccardIndex.sh";
}

void JaccardIndex::parseOutput(std::string myMaskPath) {


    myMaskPath.append(outputFileExtension);
    std::ifstream infile(myMaskPath.c_str());
    std::string line;
    float area;
    float totalArea = 0;
    int numberOfPolygonsIntersections = 0;
    //Get the total area of the intersection
    while (std::getline(infile, line)) {
        stringstream ss(line);
        ss >> area;
        totalArea += area;
        numberOfPolygonsIntersections++;
    }

    if (remove(myMaskPath.c_str()) != 0)
        perror("Comparative Analysis - HadoopGIS - Jaccard: Error deleting temporary file. Are you executing simultaneosly the same program?\n");

    float compId = diff[0];
    this->diff[0] = totalArea / (amountOfPolygons / 2);
    this->diff[1] = numberOfPolygonsIntersections;
    std::cout << "Comparative Analysis - HadoopGIS - Jaccard: CompId: " << compId << " Jaccard Index: " <<
    this->diff[0] <<
    " Number of Polygon Intersections: " << this->diff[1] <<
    std::endl;

}

void  JaccardIndex::callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                               std::string referenceMaskFileName) {
    executeScript(pathToScript, this->scriptName, pathToHadoopgisBuild, maskFileName, referenceMaskFileName);

}