//
// Created by taveira on 11/8/15.
//

#include "Hadoopgis.h"

#define ROW_MIN_DIMENSION 0
#define ROW_MAX_DIMENSION 1
#define COLUMN_MIN_DIMENSION 2
#define COLUMN_MAX_DIMENSION 3

#define LABELED_IMAGE_TYPE CV_32S

void Hadoopgis::getPolygonsFromMask(const cv::Mat &imageLabeled,
                                    std::vector<std::vector<std::vector<cv::Point> > *> *&listOfPolygons) {
    listOfPolygons = new std::vector<std::vector<std::vector<cv::Point> > *>;


    //We identify if it is a labeled mask or a binary mask by the type of the image.
    // If the image has CV_32S type, it is a labeled image, because that's the type of image we create when we parse/read the txt file containing the labels
    // If the image has CV_8U type or other, it is a binary mask, because that's the type of image that imwrite can output.

    cout << "Image Type: " << imageLabeled.type() << endl;

    //In case if it is a labeled mask, if not, go to else
    if (imageLabeled.type() == LABELED_IMAGE_TYPE) {
        //########################################## In case it is a labeled mask ##########################################
        int rows = imageLabeled.rows;
        int columns = imageLabeled.cols;
        int a, k;

        cout << "Rows: " << rows << " - Columns:" << columns << endl;

        int amount_of_labels = 0;

        //Initialize the bounding boxes
        int boundingBox[rows][4];
        for (int l = 0; l < rows; ++l) {
            boundingBox[l][ROW_MIN_DIMENSION] = rows;
            boundingBox[l][ROW_MAX_DIMENSION] = 0;
            boundingBox[l][COLUMN_MIN_DIMENSION] = columns;
            boundingBox[l][COLUMN_MAX_DIMENSION] = 0;
        }
        //Read image from text file and find the bounding boxes
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < columns; ++j) {
                k++;
                a = imageLabeled.at<int>(i, j);
                if (a != 0) {

                    if (a > amount_of_labels) amount_of_labels = a;

                    if (i < boundingBox[a][ROW_MIN_DIMENSION]) boundingBox[a][ROW_MIN_DIMENSION] = i;
                    if (i > boundingBox[a][ROW_MAX_DIMENSION]) boundingBox[a][ROW_MAX_DIMENSION] = i;

                    if (j < boundingBox[a][COLUMN_MIN_DIMENSION]) boundingBox[a][COLUMN_MIN_DIMENSION] = j;
                    if (j > boundingBox[a][COLUMN_MAX_DIMENSION]) boundingBox[a][COLUMN_MAX_DIMENSION] = j;

                }
            }
        }
        //Find the largest Bounding Box dimensions
        int maxRowSize = 0;
        int maxColumnSize = 0;
        for (int n = 1; n <= amount_of_labels; ++n) {
            int rowSize = boundingBox[n][ROW_MAX_DIMENSION] - boundingBox[n][ROW_MIN_DIMENSION] + 1;
            int columnSize = boundingBox[n][COLUMN_MAX_DIMENSION] - boundingBox[n][COLUMN_MIN_DIMENSION] + 1;
            if (maxRowSize < rowSize) maxRowSize = rowSize;
            if (maxColumnSize < columnSize) maxColumnSize = columnSize;
        }
        maxRowSize += 2; //Ensure that objects in the border of the bounding boxes have at least a pixel
        maxColumnSize += 2;


        //Create a temp image that can fit any bounding box
        cv::Mat tempMatrix = cv::Mat(maxRowSize, maxColumnSize, CV_32S, 0);

        //Copy each bounding box to the temp matrix and extract the polygons
        for (int n = 1; n <= amount_of_labels; ++n) {

            tempMatrix = cv::Mat::zeros(maxRowSize, maxColumnSize, CV_32S);
            int rowSize = boundingBox[n][ROW_MAX_DIMENSION] - boundingBox[n][ROW_MIN_DIMENSION] + 1;
            int columnSize = boundingBox[n][COLUMN_MAX_DIMENSION] - boundingBox[n][COLUMN_MIN_DIMENSION] + 1;

            cv::Mat roi = tempMatrix(cv::Rect(1, 1, columnSize, rowSize));

            //Ensure that each ROI has a border of at least 1 px for each img.
            int roiRowIndex = 0;
            int roiColumnIndex = 0;

            int i;
            int j;
            //Copy the label to the tempMatrix
            for (i = boundingBox[n][ROW_MIN_DIMENSION]; i <= boundingBox[n][ROW_MAX_DIMENSION]; ++i, ++roiRowIndex) {
                roiColumnIndex = 0;
                for (j = boundingBox[n][COLUMN_MIN_DIMENSION];
                     j <= boundingBox[n][COLUMN_MAX_DIMENSION]; ++j, ++roiColumnIndex) {
                    //Copy the label contents to the ROI
                    if (imageLabeled.at<int>(i, j) == n) {
                        roi.at<int>(roiRowIndex, roiColumnIndex) = 255;
                    } else roi.at<int>(roiRowIndex, roiColumnIndex) = 0;

                }
            }

            //Preparing for findContours
            tempMatrix.convertTo(tempMatrix, CV_8UC1, 255);

            std::vector<std::vector<cv::Point> > *cellContour;
            extractPolygons(tempMatrix, cellContour, boundingBox[n][COLUMN_MIN_DIMENSION] - 1,
                            boundingBox[n][ROW_MIN_DIMENSION] - 1, true);
            listOfPolygons->push_back(cellContour);


        }
        //Write the labeled image without the contours drawn.
//        stringstream oss;
//        oss << "reference" << ".txt" ;
//        string outputFileName (oss.str());
//        outputFileName.replace(outputFileName.find(".txt"), sizeof(".txt")-1, "_labeled.ppm");
//        imwrite(outputFileName, imageLabeled);


    } else {
        //########################################## In case it is a binary mask ##########################################

        //Extract the polygons from the image. (Non-convex polygons can be returned)
        std::vector<std::vector<cv::Point> > contours;
        cv::Mat contourOutput = imageLabeled.clone();

        cv::findContours(contourOutput, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

        /// Find the convex hull object for each contour
        vector<vector<cv::Point> > *listOfCells = new vector<vector<cv::Point> >(contours.size());
        vector<vector<cv::Point> > *hull = listOfCells;

        //vector<vector<cv::Point> > hull(contours.size());
        for (int i = 0; i < contours.size(); i++) { convexHull(cv::Mat(contours[i]), (*hull)[i], false); }
        listOfPolygons->push_back(hull);


        //Write the labeled image without the contours drawn.
//        stringstream oss;
//        oss << rand() << ".txt" ;
//        string outputFileName (oss.str());
//        outputFileName.replace(outputFileName.find(".txt"), sizeof(".txt")-1, "_generated.ppm");
//        imwrite(outputFileName, imageLabeled);


        //Draw each cell contour in the labeled image
//        int cell = 0;
//        for( ; cell < listOfPolygons->size(); cell++ )
//        {
//            cv::Scalar color( 255, 255, 255);
//            drawContours( contourOutput, *(listOfPolygons->at(cell)), -1, color, 1, 8 );
//        }
//
//        string outputFileName2 (oss.str());
//        outputFileName2.replace(outputFileName2.find(".txt"), sizeof(".txt")-1, "_generated_contour.ppm");
//        imwrite(outputFileName2, contourOutput);

    }

}


void Hadoopgis::extractPolygons(const cv::Mat &img, std::vector<std::vector<cv::Point> > *&listOfPolygons, int xOffset,
                                int yOffset, bool shouldPolygonsBeConvex) {


    //Extract the polygons from the image. (Non-convex polygons can be returned)
    std::vector<std::vector<cv::Point> > *contours = new std::vector<std::vector<cv::Point> >;

    cv::findContours(img, *contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
    //Shift each point with the bounding box offset
    for (int i = 0; i < (*contours).size(); ++i) {
        for (int j = 0; j < (*contours)[i].size(); ++j) {
            (*contours)[i][j].x = (*contours)[i][j].x + xOffset;
            (*contours)[i][j].y = (*contours)[i][j].y + yOffset;
        }
    }

    if (!shouldPolygonsBeConvex) { //Non Convex polygons (contours)
        listOfPolygons = contours;
    }
    else { // Convex polygons hull (contours)
        /// Find the convex hull object for each contour
        listOfPolygons = new vector<vector<cv::Point> >((*contours).size());
        vector<vector<cv::Point> > *hull = listOfPolygons;
        //vector<vector<cv::Point> > hull(contours.size());
        for (int i = 0; i < (*contours).size(); i++) { convexHull(cv::Mat((*contours)[i]), (*hull)[i], false); }
    }
}

void Hadoopgis::convertPolygonToHadoopgisInput(std::vector<std::vector<std::vector<cv::Point> > *> *listOfhulls,
                                               std::ofstream &ss,
                                               double &area) {

    for (int k = 0; k < (*listOfhulls).size(); ++k) {
        std::vector<std::vector<cv::Point> > *hull = (*listOfhulls)[k];

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
//    getPolygonsFromMask(image1, this->listOfPolygons[0]);
//    getPolygonsFromMask(image2, this->listOfPolygons[1]);
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
