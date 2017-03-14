#ifndef TASK_SEGMENTATION_H_
#define TASK_SEGMENTATION_H_

#include "Task.h"
#include "DenseDataRegion2D.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "FileUtils.h"


class TaskSegmentation : public Task {
private:
    DenseDataRegion2D *bgr;
    DenseDataRegion2D *mask;

    unsigned char blue, green, red;
    double T1, T2;
    unsigned char G1, G2;
    int minSize, maxSize, minSizePl, minSizeSeg, maxSizeSeg;
    int fillHolesConnectivity, reconConnectivity, watershedConnectivity;
    uint64_t *executionTime;
public:
    TaskSegmentation(DenseDataRegion2D *bgr, DenseDataRegion2D *mask, unsigned char blue, unsigned char green,
                     unsigned char red, double T1, double T2, unsigned char G1, unsigned char G2, int minSize,
                     int maxSize, int minSizePl, int minSizeSeg, int maxSizeSeg, int fillHolesConnectivity,
                     int reconConnectivity, int watershedConnectivity, uint64_t *executionTime);

    virtual ~TaskSegmentation();

    bool run(int procType = ExecEngineConstants::CPU, int tid = 0);
};

#endif /* TASK_SEGMENTATION_H_ */
