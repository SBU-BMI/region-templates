#include "TaskNormalization.h"

TaskNormalization::TaskNormalization(DenseDataRegion2D *raw, DenseDataRegion2D *bgr, float targetMean[3]) {
    this->bgr = bgr;
    this->raw = raw;
    for (int i = 0; i < 3; i++) this->targetMean[i] = targetMean[i];
}

TaskNormalization::~TaskNormalization() {
    if (raw != NULL) delete raw;
}

bool TaskNormalization::run(int procType, int tid) {
    cv::Mat inputImage = this->raw->getData();
    // target values computed from the reference image

    float targetStd[3] = {0.148816049099, 0.0257016178221, 0.0088480217382};

    cv::Mat NormalizedImg;
    std::cout << "Normalization:: " << " targetMean: " << targetMean[0] << ":" << targetMean[1] << ":" <<
    targetMean[2] << std::endl;
    uint64_t t1 = Util::ClockGetTimeProfile();
    if (inputImage.rows > 0)
        NormalizedImg = ::nscale::Normalization::normalization(inputImage, targetMean, targetStd);
    else
        std::cout << "Normalization: input data NULL" << std::endl;
    uint64_t t2 = Util::ClockGetTimeProfile();

    this->bgr->setData(NormalizedImg);

    std::cout << "Task Normalization time elapsed: " << t2 - t1 << std::endl;
}
