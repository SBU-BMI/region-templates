#ifndef DENSE_SVS_DATA_REGION_2D_H_
#define DENSE_SVS_DATA_REGION_2D_H_

#include "ExecutionEngine.h"
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

    void getMatData(ExecutionEngine* env);
    void getMatMetadata(ExecutionEngine* env);

public:
    DenseSvsDataRegion2D(cv::Rect_<int64_t> roi);
    ~DenseSvsDataRegion2D();

    cv::Mat getData(ExecutionEngine* env);

    // int serialize(char* buff);
    // int deserialize(char* buff);
    // int serializationSize();

#endif /* DENSE_SVS_DATA_REGION_2D_H_ */
