#ifndef DENSE_SVS_DATA_REGION_2D_H_
#define DENSE_SVS_DATA_REGION_2D_H_

#include "ExecutionEngine.h"
#include "DenseDataRegion2D.h"

#include "openslide.h"
#include <opencv/cv.hpp>

class DenseSvsDataRegion2D: public DenseDataRegion2D {
public:
    DenseSvsDataRegion2D(cv::Rect_<int64_t> roi);

    cv::Mat getData(ExecutionEngine* env);
};

#endif /* DENSE_SVS_DATA_REGION_2D_H_ */
