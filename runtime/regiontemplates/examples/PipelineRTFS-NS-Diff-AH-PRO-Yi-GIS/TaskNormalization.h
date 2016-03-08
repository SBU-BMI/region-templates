#ifndef TASK_NORMALIZATION_H_
#define TASK_NORMALIZATION_H_

#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "Normalization.h"
#include "Util.h"
#include "FileUtils.h"


class TaskNormalization : public Task {
private:
    DenseDataRegion2D *raw;
    DenseDataRegion2D *bgr;

    float targetMean[3];

public:
    TaskNormalization(DenseDataRegion2D *raw, DenseDataRegion2D *bgr, float targetMean[3]);

    virtual ~TaskNormalization();

    bool run(int procType = ExecEngineConstants::CPU, int tid = 0);
};

#endif /* TASK_NORMALIZATION_H_ */
