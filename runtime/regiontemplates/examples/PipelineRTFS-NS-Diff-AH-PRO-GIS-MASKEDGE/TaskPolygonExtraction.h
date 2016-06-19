//
// Created by taveira on 6/5/16.
//

#ifndef RUNTIME_TASKPOLYGONEXTRACTION_H
#define RUNTIME_TASKPOLYGONEXTRACTION_H

#include <regiontemplates/PolygonListDataRegion.h>
#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "Normalization.h"
#include "Util.h"
#include "FileUtils.h"


class TaskPolygonExtraction : public Task {
private:
    DenseDataRegion2D *refmask;
    PolygonListDataRegion *polygonListDataRegion;

public:
    TaskPolygonExtraction(DenseDataRegion2D *refmask, PolygonListDataRegion *polygonListDataRegion);

    virtual ~TaskPolygonExtraction();

    bool run(int procType = ExecEngineConstants::CPU, int tid = 0);
};

#endif //RUNTIME_TASKPOLYGONEXTRACTION_H
