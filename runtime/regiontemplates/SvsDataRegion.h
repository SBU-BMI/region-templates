#ifndef SVS_DATA_REGION_H_
#define SVS_DATA_REGION_H_

#include "ExecutionEngine.h"
#include "DenseDataRegion2D.h"

#include "openslide.h"
#include <opencv/cv.hpp>

class SvsDataRegion: public DataRegion {
public:
    SvsDataRegion();

    // Lazily gets the mat data of the internal roi on a svs file on the
    // inputed environment (i.e., execution node)
    cv::Mat getData(ExecutionEngine* env);
};

#endif /* SVS_DATA_REGION_H_ */
