//
// Created by taveira on 12/7/15.
//

#ifndef RUNTIME_KNNRESULT_H
#define RUNTIME_KNNRESULT_H

#include <iostream>
#include <stdio.h>
#include <vector>
#include <opencv2/core/core.hpp>

class KnnResult {

public:
    std::vector<std::vector<std::vector<cv::Point> > *> *listOfPolygons[2];
    std::vector<long> polygonsRelationships[2];
    std::vector<double> distance;
};


#endif //RUNTIME_KNNRESULT_H
