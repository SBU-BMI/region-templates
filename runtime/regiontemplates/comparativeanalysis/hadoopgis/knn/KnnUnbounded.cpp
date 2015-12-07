//
// Created by taveira on 12/4/15.
//

#include "KnnUnbounded.h"

KnnUnbounded::KnnUnbounded(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *diffPixels, long k) {
    this->dr1 = dr1;
    this->dr2 = dr2;
    this->diff = diffPixels;
    this->k = k;
    this->scriptName = "KnnUnbounded.sh";
}

void KnnUnbounded::parseOutput(std::string myMaskPath) {


    myMaskPath.append(outputFileExtension);
    std::ifstream infile(myMaskPath.c_str());
    std::string line;
    float area;
    float totalArea = 0;
    int numberOfPolygonsIntersections = 0;

    vector<long> knn[2];
    vector<double> distance;

    //Get the polygons and the polygons distance.
    while (std::getline(infile, line)) {
        long polygonId;
        double dist;
        infile >> polygonId;
        knn[0].push_back(polygonId);
        infile >> polygonId;
        knn[1].push_back(polygonId);
        infile >> dist;
        distance.push_back(dist);
        numberOfPolygonsIntersections++;
    }
    //Don't remove this - the while above inserts a last line that should be disconsidered.
    knn[0].pop_back();
    knn[1].pop_back();
    distance.pop_back();
    //
    for (int i = 0; i < knn[0].size(); ++i) {
        std::cout << knn[0].at(i) << "\t" << knn[1].at(i) << "\t" << distance.at(i) << endl;

    }
//    if (remove(myMaskPath.c_str()) != 0)
//        perror("Comparative Analysis - HadoopGIS - KNN: Error deleting temporary file. Are you executing simultaneosly the same program?\n");

    float compId = diff[0];
    this->diff[0] = totalArea;
    this->diff[1] = numberOfPolygonsIntersections;
//    std::cout << "CompId: " << compId << " Dice Coefficient: " << this->diff[0] <<
//    " Number of Polygons Intersections: " << this->diff[1] <<
//    std::endl;


}

void  KnnUnbounded::callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                               std::string referenceMaskFileName) {
    std::cout << "ALI!!!!!!!!!!!!!!!!!!!!!" << endl << endl << endl;
    KnnUnbounded::executeScript(pathToScript, this->scriptName, pathToHadoopgisBuild, maskFileName,
                                referenceMaskFileName);

}

void KnnUnbounded::executeScript(std::string pathToScript, std::string scriptName, std::string pathToHgisBuild,
                                 std::string maskFileName,
                                 std::string referenceMaskFileName) {
    std::cout << "AQUI!!!!!!!!!!!!!!!!!!!!!" << endl << endl << endl;
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