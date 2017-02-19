#include "TaskSegmentation.h"
//itk
#include "itkImage.h"
#include "itkRGBPixel.h"
#include "itkImageFileWriter.h"
#include "itkOtsuThresholdImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkOpenCVImageBridge.h"


#include "utilityTileAnalysis.h"

TaskSegmentation::TaskSegmentation(DenseDataRegion2D *bgr, DenseDataRegion2D *mask, float otsuRatio,
                                   float curvatureWeight, float sizeThld, float sizeUpperThld, float mpp,
                                   float mskernel, int levelSetNumberOfIteration, int declumpingType,
                                   int *executionTime) {
    this->bgr = bgr;
    this->mask = mask;
    this->otsuRatio = otsuRatio;
    this->curvatureWeight = curvatureWeight;
    this->sizeThld = sizeThld;
    this->sizeUpperThld = sizeUpperThld;
    this->mpp = mpp;
    this->mskernel = mskernel;
    this->levelSetNumberOfIteration = levelSetNumberOfIteration;
    this->declumpingType = declumpingType;
    this->executionTime = executionTime;
}

TaskSegmentation::~TaskSegmentation() {
    if (bgr != NULL) delete bgr;
}

bool TaskSegmentation::run(int procType, int tid) {
    cv::Mat inputImage = this->bgr->getData();
    cv::Mat outMask;

    uint64_t t1 = Util::ClockGetTimeProfile();
    std::string imageName = this->bgr->getInputFileName();
    std::cout << "TaskSegmentationFileName: " << imageName << std::endl;

    if (inputImage.rows > 0)
        outMask = ImagenomicAnalytics::TileAnalysis::processTileCV(inputImage, otsuRatio, curvatureWeight, sizeThld,
                                                                   sizeUpperThld, mpp, mskernel,
                                                                   levelSetNumberOfIteration, declumpingType);
    else
        std::cout << "Segmentation: input data NULL" << std::endl;
    this->mask->setData(outMask);

    uint64_t t2 = Util::ClockGetTimeProfile();

    this->executionTime[0] = (int) t2 - t1; // Execution Time


    //std::cout << "Task Segmentation image name: " << imageName << std::endl;
    std::string key("image");
    std::size_t found = imageName.rfind(key);

    std::string lastPoint(".");
    std::size_t lastPointFound = imageName.rfind(lastPoint);

    std::string result;
    if (found != std::string::npos)
        result = imageName.substr(found + key.size(), lastPointFound - (found + key.size()));

    //std::cout << "###IMAGE: " << result << endl;

    this->executionTime[1] = atoi(result.c_str());

    std::cout << "Task Segmentation time elapsed: " << this->executionTime[0] << std::endl;
    std::cout << "Task Segmentation image: " << this->executionTime[1] << std::endl;

}
