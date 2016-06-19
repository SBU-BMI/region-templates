//
// Created by taveira on 6/4/16.
//

#ifndef RUNTIME_POLYGONEXTRACTIONCOMP_H
#define RUNTIME_POLYGONEXTRACTIONCOMP_H

#include "RTPipelineComponentBase.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "Util.h"
#include "FileUtils.h"

class PolygonExtractionComp : public RTPipelineComponentBase {
private:
    cv::Mat inputImage;
    std::vector<std::vector<cv::Point> > listOfPolygons;

public:
    PolygonExtractionComp();

    virtual ~PolygonExtractionComp();

    int run();

};


#endif //RUNTIME_POLYGONEXTRACTIONCOMP_H
