//
// Created by taveira on 11/10/15.
//

#include <regiontemplates/PolygonListDataRegion.h>
#include "TaskDiceDiffPolyList.h"


TaskDiceDiffPolyList::TaskDiceDiffPolyList(DenseDataRegion2D *tileMask,
                                           PolygonListDataRegion *polyListDRFromReferenceMask,
                                           float *diffPixels) {
    this->dr1 = tileMask;
    this->dr2 = NULL;

    //Extract the polygons from the tile
    getPolygonsFromMask(this->dr1->getData(), this->listOfPolygons[0]);

    //Save the reference for the list of polygons from the reference mask
    std::vector<std::vector<cv::Point> > *pointer = new std::vector<std::vector<cv::Point> >(
            polyListDRFromReferenceMask->getData());
    //std::vector<std::vector<cv::Point> > listOfPolygonsFromReferenceMask (polyListDRFromReferenceMask->getData());
    this->listOfPolygons[1] = pointer;

    this->diff = diffPixels;
    this->scriptName = "SpatialQuery.sh";
}

void TaskDiceDiffPolyList::parseOutput(std::string myMaskPath, double a1, double a2) {


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
        perror("Comparative Analysis - TaskDiceDiffPolyList: Error deleting temporary file. Are you executing simultaneosly the same program?\n");

    float totalArea = (float) (a1 + a2);
    //float dice = 2 * totalIntersectArea / (totalArea);
    float compId = diff[0];
    this->diff[0] = totalIntersectArea;
    this->diff[1] = (float) a2;
    this->diff[2] = (float) a1;
    std::cout << "Comparative Analysis - TaskDiceDiffPolyList: CompId: " << compId <<
    " Intersect Area: " << totalIntersectArea << " Total polylist area:" << this->diff[1] << " Tile area:" <<
    this->diff[2] <<
    std::endl;


}

void  TaskDiceDiffPolyList::callScript(std::string pathToScript, std::string pathToHadoopgisBuild,
                                       std::string maskFileName,
                                       std::string referenceMaskFileName) {
    executeScript(pathToScript, this->scriptName, pathToHadoopgisBuild, maskFileName, referenceMaskFileName);

}