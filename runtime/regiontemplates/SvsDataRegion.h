#ifndef SVS_DATA_REGION_H_
#define SVS_DATA_REGION_H_

#include "ExecutionEngine.h"
#include "DenseDataRegion2D.h"

#include "openslide.h"
#include <opencv/cv.hpp>

class SvsDataRegion: public DataRegion {
public:
    SvsDataRegion();
    
    void setRoi(cv::Rect_<int64_t> roi);
    void printRoi(); // debugging method

    int serialize(char* buff) override;
    int deserialize(char* buff) override;
    int serializationSize() override;

    // Lazily gets the mat data of the internal roi on a svs file on the
    // inputed environment (i.e., execution node)
    cv::Mat getData(ExecutionEngine* env);
private:
    // ROI bounding box for svs files
    cv::Rect_<int64_t> roi;
};

#endif /* SVS_DATA_REGION_H_ */
