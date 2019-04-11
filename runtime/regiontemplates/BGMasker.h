#ifndef BG_MASKER_H_
#define BG_MASKER_H_

#include <opencv/cv.hpp>

class BGMasker {
public:
    BGMasker() {};

    virtual cv::Mat bgMask(cv::Mat img) = 0;
};

#endif
