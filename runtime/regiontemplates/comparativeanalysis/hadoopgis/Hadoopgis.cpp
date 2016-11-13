//
// Created by taveira on 11/8/15.
//

#include "Hadoopgis.h"


void Hadoopgis::getPolygonsFromLabeledMask(const cv::Mat &img, std::vector<std::vector<cv::Point> > *&listOfPolygons) {


    //Extract the polygons from the image. (Non-convex polygons can be returned)
    std::vector<std::vector<cv::Point> > contours;
    cv::Mat contourOutput = img.clone();
    cv::findContours(contourOutput, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

    /// Find the convex hull object for each contour
    listOfPolygons = new vector<vector<cv::Point> >(contours.size());
    vector<vector<cv::Point> > *hull = listOfPolygons;
    //vector<vector<cv::Point> > hull(contours.size());
    for (int i = 0; i < contours.size(); i++) { convexHull(cv::Mat(contours[i]), (*hull)[i], false); }

}

void Hadoopgis::getPolygonsFromBinaryMask(const cv::Mat &img, std::vector<std::vector<cv::Point> > *&listOfPolygons) {


    //Extract the polygons from the image. (Non-convex polygons can be returned)
    std::vector<std::vector<cv::Point> > contours;
    cv::Mat contourOutput = img.clone();
    cv::findContours(contourOutput, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

    /// Find the convex hull object for each contour
    listOfPolygons = new vector<vector<cv::Point> >(contours.size());
    vector<vector<cv::Point> > *hull = listOfPolygons;
    //vector<vector<cv::Point> > hull(contours.size());
    for (int i = 0; i < contours.size(); i++) { convexHull(cv::Mat(contours[i]), (*hull)[i], false); }

}

void Hadoopgis::convertPolygonToHadoopgisInput(std::vector<std::vector<cv::Point> > *hull, std::ofstream &ss,
                                               double &area) {

    //Output the polygons to a format that Hadoopgis can read
    //Each cell corresponds to a line in the format:
    //CELL ID   POLYGON((Point1,Point2,...,PointN))
    for (int i = 0; i < (*hull).size(); i++) {
        //Sort the points of the polygon
        //sort(hull[i].begin(), hull[i].end(), compare_points);
        ss << i + 1 << "\t" << "POLYGON((";
        for (int j = 0; j < (*hull)[i].size(); j++) // run until j < contours[i].size();
        {
            if (j > 0)ss << ",";
            ss << (*hull)[i][j].x << " " << (*hull)[i][j].y; //do whatever
        }
        //close the polygon with the first vertex.
        ss << "," << (*hull)[i][0].x << " " << (*hull)[i][0].y << "))";
//        for (int k = 1; k <=15 ; ++k) {
//            ss << "\t" << k%9;
//        }
        ss << endl;
        area += fabs(contourArea(cv::Mat((*hull)[i]))); //Get the sum of the polygon areas.
    }

}

bool Hadoopgis::compare_points(const cv::Point &e1, const cv::Point &e2) {
    if (e1.x != e2.x)
        return (e1.x < e2.x);
    return (e1.y < e2.y);
}

bool Hadoopgis::run(int procType, int tid) {

    uint64_t t1 = Util::ClockGetTimeProfile();
    double compId = diff[0];
//    cv::Mat image1 = this->dr1->getData();
//    cv::Mat image2 = this->dr2->getData();
    //Local unique ID - This ID is what distinguishes a temp file of a thread/process from the temp file of another thread/process
    //TODO: ADD A SUFFIX TO THE ID TO PERMIT SIMULTANEOUS RUNS OF HadoopGIS
    std::stringstream myId;
    myId << compId;

    //The program must have write permission to temp folder.
    string tempPath = STRINGIFY(WRITE_ENABLED_TEMP_PATH);
    //Check if the path to tmp folder ends with '/'
    if (tempPath.at(tempPath.size() - 1) != '/') {
        tempPath.append("/");
    }

    //File names
    string myMaskFileName = "maskPolygons";
    string referenceMaskFileName = "referencePolygons";

    myMaskFileName.append(myId.str());
    referenceMaskFileName.append(myId.str());
    //myMaskFileName.append(extension);
    //referenceMaskFileName.append(extension);

    //File output of the list of polygons
    ofstream myMaskFile, referenceMaskFile;
    string myMaskPath = tempPath + myMaskFileName;
    string referenceMaskPath = tempPath + referenceMaskFileName;
    myMaskFile.open(myMaskPath.c_str());
    referenceMaskFile.open(referenceMaskPath.c_str());
//    getPolygonsFromLabeledMask(image1, this->listOfPolygons[0]);
//    getPolygonsFromLabeledMask(image2, this->listOfPolygons[1]);
    double area1 = 0;
    convertPolygonToHadoopgisInput(this->listOfPolygons[0], myMaskFile, area1);
    double area2 = 0;
    convertPolygonToHadoopgisInput(this->listOfPolygons[1], referenceMaskFile, area2);
    myMaskFile.close();
    referenceMaskFile.close();

    string pathToScripts = STRINGIFY(HADOOPGIS_SCRIPTS_DIR);
    //Path to Hadoopgis Build folder.
    string pathToHadoopgisBuild = STRINGIFY(HADOOPGIS_BUILD_DIR);

    callScript(pathToScripts, pathToHadoopgisBuild, myMaskFileName, referenceMaskFileName);

    parseOutput(myMaskPath, area1, area2);

    uint64_t t2 = Util::ClockGetTimeProfile();

    std::cout << "\t(**query)Task Diff Computation time elapsed: " << t2 - t1 << std::endl;

}

void Hadoopgis::executeScript(std::string pathToScript, std::string scriptName, std::string pathToHgisBuild,
                              std::string maskFileName,
                              std::string referenceMaskFileName) {

    // Call the script passing the polygons as arguments
    // Sends 4 params:  1 - path to hadoopgis,
    //                  2 - path to tempfolder(write-permission)
    //                  3 - mask polygons and
    //                  4 - referenceMask polygons

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
    //cout << command.c_str();
    //Execute script
    int returnOfSystemCall = system(command.c_str());

}
