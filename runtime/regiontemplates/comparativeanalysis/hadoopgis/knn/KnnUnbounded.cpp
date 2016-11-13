//
// Created by taveira on 12/4/15.
//

#include "KnnUnbounded.h"

KnnUnbounded::KnnUnbounded(std::vector<std::vector<cv::Point> > *list1, std::vector<std::vector<cv::Point> > *list2,
                           float *id, long k) {
    this->dr1 = NULL;
    this->dr2 = NULL;
    //List of Polygons 1
    this->listOfPolygons[0] = list1;
    //List of Polygons 2
    this->listOfPolygons[1] = list2;
    this->k = k;
    this->diff = id;
    this->scriptName = "KnnUnbounded.sh";
}

KnnUnbounded::KnnUnbounded(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *id, long k) {
    this->dr1 = dr1;
    this->dr2 = dr2;
    getPolygonsFromLabeledMask(this->dr1->getData(), this->listOfPolygons[0]);
    getPolygonsFromLabeledMask(this->dr2->getData(), this->listOfPolygons[1]);
    this->k = k;
    this->diff = id;
    this->scriptName = "KnnUnbounded.sh";
}

void KnnUnbounded::parseOutput(std::string myMaskPath, double area1, double area2) {


    myMaskPath.append(outputFileExtension);
    std::ifstream infile(myMaskPath.c_str());
    std::string line;
    float area;
    float totalArea = 0;
    int numberOfPolygonsIntersections = 0;

    result = new KnnResult();
    //Get the polygons and the polygons distance.
    while (std::getline(infile, line)) {
        stringstream ss(line);
        long polygonId;
        double dist;
        ss >> polygonId;
        result->polygonsRelationships[0].push_back(polygonId);
        ss >> polygonId;
        result->polygonsRelationships[1].push_back(polygonId);
        ss >> dist;
        totalArea += dist;
        result->distance.push_back(dist);
        numberOfPolygonsIntersections++;
    }

    //Storing the original query polygons in the result object
    result->listOfPolygons[0] = this->listOfPolygons[0];
    result->listOfPolygons[1] = this->listOfPolygons[1];


    //Printing the result of KNN
    std::cout << "Polygon ID (from list 1)\tPolygon ID (from list 2)\tDistance Between Them" << endl;
    for (int i = 0; i < result->polygonsRelationships[0].size(); ++i) {
        std::cout << result->polygonsRelationships[0].at(i) << "\t\t\t\t" << result->polygonsRelationships[1].at(i) <<
        "\t\t\t\t" <<
        result->distance.at(i) << endl;

    }

    if (remove(myMaskPath.c_str()) != 0)
        perror("Comparative Analysis - HadoopGIS - KNN: Error deleting temporary file. Are you executing simultaneosly the same program?\n");

    float compId = diff[0];
    this->diff[0] = totalArea;
    this->diff[1] = numberOfPolygonsIntersections;
    std::cout << "Comparative Analysis - HadoopGIS - KNN: CompId: " << compId <<
    " Sum of Distances Between Polygons: " << this->diff[0] <<
    " Number of Polygons Relations: " << this->diff[1] <<
    std::endl;


}

//KnnResult *KnnUnbounded::getKnnResult() {
//    return result;
//}

void  KnnUnbounded::callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                               std::string referenceMaskFileName) {
    KnnUnbounded::executeScript(pathToScript, this->scriptName, pathToHadoopgisBuild, maskFileName,
                                referenceMaskFileName);

}

void KnnUnbounded::executeScript(std::string pathToScript, std::string scriptName, std::string pathToHgisBuild,
                                 std::string maskFileName,
                                 std::string referenceMaskFileName) {
    // Call the script passing the polygons as arguments
    // Sends 5 params:  1 - path to hadoopgis,
    //                  2 - path to tempfolder(write-permission)
    //                  3 - mask polygons and
    //                  4 - referenceMask polygons
    //                  5 - number of k elements

    //string scriptName = "MaskIntersection.sh";
    string command = string("bash ");

    //Check if the path to script ends with '/'
    if (pathToScript.at(pathToScript.size() - 1) != '/') {
        pathToScript.append("/");
    }

    //Check if the pathToHadoopgisBuild ends with '/'
    if (pathToHgisBuild.at(pathToHgisBuild.size() - 1) != '/') {
        pathToHgisBuild.append("/");
    }

    string tempPath = STRINGIFY(WRITE_ENABLED_TEMP_PATH);
    //Check if the path to tmp folder ends with '/'
    if (tempPath.at(tempPath.size() - 1) != '/') {
        tempPath.append("/");
    }

    //Build the command:
    //The Script (Query) Path
    command.append(pathToScript);
    command.append(scriptName);
    command.append(space);
    //Arg1 - Path to Hadoopgis Build folder
    command.append(pathToHgisBuild);
    command.append(space);
    //Arg2 - Path to Temp Folder
    command.append(tempPath);
    command.append(space);
    //Arg3 - Path to MaskFile containing the Mask's polygons
    command.append(maskFileName);
    command.append(space);
    //Arg4 - Path to ReferenceMaskFile containing the Reference Mask's polygons
    command.append(referenceMaskFileName);
    command.append(space);
    //Arg5 - Number of K elements
    std::stringstream kelements;
    std::string kelemnts;
    kelements << k;
    kelements >> kelemnts;
    command.append(kelemnts);

    //cout << command.c_str();
    //Execute script
    int returnOfSystemCall = system(command.c_str());

}