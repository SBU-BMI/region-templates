//
// Created by taveira on 11/25/15.
//

#ifndef RUNTIME_PIXELCOMPARE_H
#define RUNTIME_PIXELCOMPARE_H


#include "../TaskDiffMask.h"

class PixelCompare : public TaskDiffMask {
public:
    PixelCompare(DenseDataRegion2D *dr1, DenseDataRegion2D *dr2, float *diffPixels);

    bool run(int procType = ExecEngineConstants::CPU, int tid = 0);


};


#endif //RUNTIME_PIXELCOMPARE_H
