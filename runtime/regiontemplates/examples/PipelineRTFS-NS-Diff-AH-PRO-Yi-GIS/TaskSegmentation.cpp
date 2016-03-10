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
                                   float mskernel, int levelSetNumberOfIteration) {
    this->bgr = bgr;
    this->mask = mask;
    this->otsuRatio = otsuRatio;
    this->curvatureWeight = curvatureWeight;
    this->sizeThld = sizeThld;
    this->sizeUpperThld = sizeUpperThld;
    this->mpp = mpp;
    this->mskernel = mskernel;
    this->levelSetNumberOfIteration = levelSetNumberOfIteration;
}

TaskSegmentation::~TaskSegmentation() {
    if (bgr != NULL) delete bgr;
}

bool TaskSegmentation::run(int procType, int tid) {
    cv::Mat inputImage = this->bgr->getData();
    cv::Mat outMask;

    uint64_t t1 = Util::ClockGetTimeProfile();


//	cv::Mat seg = processTile(thisTile, otsuRatio, curvatureWeight, sizeThld, sizeUpperThld, mpp);
    if (inputImage.rows > 0)
        outMask = ImagenomicAnalytics::TileAnalysis::processTileCV(inputImage, otsuRatio, curvatureWeight, sizeThld,
                                                                   sizeUpperThld, mpp, mskernel,
                                                                   levelSetNumberOfIteration);
//		outMask = processTile(inputImage, otsuRatio, curvatureWeight, sizeThld, sizeUpperThld, 0.25);
    else
        std::cout << "Segmentation: input data NULL" << std::endl;
    this->mask->setData(outMask);

    uint64_t t2 = Util::ClockGetTimeProfile();

    std::cout << "Task Segmentation time elapsed: " << t2 - t1 << std::endl;
}
