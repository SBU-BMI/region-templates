#include "TaskSegmentation.h"

//itk
#include "itkImage.h"
#include "itkRGBPixel.h"
#include "itkImageFileWriter.h"
#include "itkOtsuThresholdImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkOpenCVImageBridge.h"


#include "utilityTileAnalysis.h"


TaskSegmentation::TaskSegmentation(DenseDataRegion2D* bgr, DenseDataRegion2D* mask) {
	this->bgr = bgr;
	this->mask = mask;
}

TaskSegmentation::~TaskSegmentation() {
	if(bgr != NULL) delete bgr;
}

bool TaskSegmentation::run(int procType, int tid) {
	cv::Mat inputImage = this->bgr->getData();
	cv::Mat outMask;

	uint64_t t1 = Util::ClockGetTimeProfile();

	if(inputImage.rows > 0){

        float otsuRatio = 1.0;
        double curvatureWeight = 0.8;
        float sizeThld = 3;
        float sizeUpperThld = 200;
        double mpp = 0.25;
        float msKernel = 20.0;
        int levelsetNumberOfIteration = 100;

		outMask = ImagenomicAnalytics::TileAnalysis::processTileCV(
                      inputImage,
                      otsuRatio,
                      curvatureWeight,
                      sizeThld,
                      sizeUpperThld,
                      mpp,
                      msKernel,
                      levelsetNumberOfIteration);


//Use these to output mask overlay:
//            itkRGBImageType::Pointer overlay = ImagenomicAnalytics::ScalarImage::generateSegmentationOverlay<char>(
//                    thisTileItk, nucleusBinaryMask);

//            ImagenomicAnalytics::IO::writeImage<itkRGBImageType>(overlay, outputOverlayName, 0);
    }
	else
		std::cout <<"Segmentation: input data NULL"<< std::endl;


    //imwrite ("/lustre/atlas/proj-shared/csc143/lot/u24/test/DB-rt-adios/masktest.png", outMask);


	this->mask->setData(outMask);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}
