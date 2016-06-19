//
// Created by taveira on 6/2/16.
//

#ifndef RUNTIME_POLYGONLISTDATAREGION_H
#define RUNTIME_POLYGONLISTDATAREGION_H

#include "DataRegion.h"
#include <string>
#include <iostream>

// OpenCV library includes
#include "cv.hpp"

//#include <cv.h>
#include <highgui.h>
#include <vector>

//using namespace cv;
#include "opencv2/gpu/gpu.hpp"

class PolygonListDataRegion : public DataRegion {
private:
    std::vector<std::vector<cv::Point> > listOfPolygons;


public:
    PolygonListDataRegion();

    virtual ~PolygonListDataRegion();

    DataRegion *clone(bool copyData);

    void setData(std::vector<std::vector<cv::Point> >);

    std::vector<std::vector<cv::Point> > getData();

    bool empty();

    long getDataSize();

};


#endif //RUNTIME_POLYGONLISTDATAREGION_H
