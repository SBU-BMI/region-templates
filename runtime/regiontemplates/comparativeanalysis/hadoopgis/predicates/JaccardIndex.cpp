//
// Created by taveira on 11/10/15.
//

#include "JaccardIndex.h"

JaccardIndex::JaccardIndex(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *diffPixels) {
    this->dr1 = dr1;
    this->dr2 = dr2;
    getPolygonsFromMask(this->dr1->getData(), this->listOfPolygons[0]);
    getPolygonsFromMask(this->dr2->getData(), this->listOfPolygons[1]);
    this->diff = diffPixels;
    this->scriptName = "JaccardIndex.sh";
}

void JaccardIndex::parseOutput(std::string myMaskPath, double a1, double a2) {


    myMaskPath.append(outputFileExtension);
    std::ifstream infile(myMaskPath.c_str());
    std::string line;
    float intersectArea;
    float totalIntersectArea = 0;
    float area1;
    float totalArea1 = 0;
    float area2;
    float totalArea2 = 0;
    float unionArea;
    float totalUnionArea = 0;

    //Get the total area of the intersection
    while (std::getline(infile, line)) {
        stringstream ss(line);
        ss >> intersectArea;
        totalIntersectArea += intersectArea;
        ss >> area1;
        totalArea1 += area1;
        ss >> area2;
        totalArea2 += area2;
        ss >> unionArea;
        totalUnionArea += unionArea;


    }

    if (remove(myMaskPath.c_str()) != 0)
        perror("Comparative Analysis - HadoopGIS - Jaccard: Error deleting temporary file. Are you executing simultaneosly the same program?\n");
    float totalArea = (float) (a1 + a2);
    float jaccard = totalIntersectArea / (totalArea - totalIntersectArea);
    float compId = diff[0];
    this->diff[0] = jaccard;
    this->diff[1] = totalArea;
    std::cout << "Comparative Analysis - HadoopGIS - Jaccard: CompId: " << compId << " Jaccard Index: " <<
    this->diff[0] <<
    " Intersect Area: " << totalIntersectArea << " Total sets area:" << this->diff[1] <<
    std::endl;


}

void  JaccardIndex::callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                               std::string referenceMaskFileName) {
    executeScript(pathToScript, this->scriptName, pathToHadoopgisBuild, maskFileName, referenceMaskFileName);

}