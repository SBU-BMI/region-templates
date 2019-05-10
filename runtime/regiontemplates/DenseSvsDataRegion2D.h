#ifndef DENSE_SVS_DATA_REGION_2D_H_
#define DENSE_SVS_DATA_REGION_2D_H_

#include "DenseDataRegion2D.h"

#include "openslide.h"
#include <opencv/cv.hpp>

class DenseSvsDataRegion2D: public DenseDataRegion2D {
private:
    cv::Rect_<int64_t> roi;
    bool hasData;
    bool hasMetadata;
    int64_t w, h;
    int32_t maxLevel;
    openslide_t* svsFile;

    void getMatData();
    void getMatMetadata();

public:
    DenseSvsDataRegion2D(cv::Rect_<int64_t> roi);
    ~DenseSvsDataRegion2D();

    cv::Mat getData();

    int getXDimensionSize();
    int getYDimensionSize();
};

#endif /* DENSE_SVS_DATA_REGION_2D_H_ */
