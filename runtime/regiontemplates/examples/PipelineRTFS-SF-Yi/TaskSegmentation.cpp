#include "TaskSegmentation.h"

//itk
#include "itkImage.h"
#include "itkRGBPixel.h"
#include "itkImageFileWriter.h"
#include "itkOtsuThresholdImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkOpenCVImageBridge.h"


#include "utilityTileAnalysis.h"


#include "HistologicalEntities.h"

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

//Jun's algorithm
        int segmentationExecCode = ::nscale::HistologicalEntities::segmentNuclei(inputImage, outMask);

        if (segmentationExecCode == ::nscale::HistologicalEntities::SUCCESS){
            std::cout << "Jun's segmentation reported success\n";
        } else {
            std::cout << "Jun's segmentation did not report success\n";
            // create an empty mask the same size as the input so as not
            // to disrupt the rest of the processing
            outMask = cv::Mat::zeros(inputImage.rows, inputImage.cols, CV_8UC1);
        }

//Yi's algorithm
//		outMask = ImagenomicAnalytics::TileAnalysis::processTileCV(
//                      inputImage,
//                      this->otsuRatio,
//                      this->curvatureWeight,
//                      this->sizeThld,
//                      this->sizeUpperThld,
//                      this->mpp,
//                      this->msKernel,
//                      this->levelsetNumberOfIteration);


        // Convert openCV to itk
        typedef itk::RGBPixel<unsigned char> RGBPixelType;
        typedef itk::Image<RGBPixelType, 2> itkRGBImageType;
        typedef unsigned char                PixelType;
        typedef itk::Image<PixelType, 2>     ImageType;

        itkRGBImageType::Pointer itkInputImage = itk::OpenCVImageBridge::CVMatToITKImage<itkRGBImageType>(inputImage);

// Write out mask from openCV as a png
        std::stringstream cmname_ss;
        cmname_ss << this->bgr->getId() << "_mask.png";
        imwrite(cmname_ss.str().c_str(), outMask);

        ImageType::Pointer itkMask = itk::OpenCVImageBridge::CVMatToITKImage<ImageType>(outMask);

        itkRGBImageType::Pointer overlay = ImagenomicAnalytics::ScalarImage::generateSegmentationOverlay<unsigned char>(itkInputImage, itkMask);

        std::stringstream name_ss;
        name_ss << this->bgr->getId() << "_overlay.png";

        ImagenomicAnalytics::IO::writeImage<itkRGBImageType>(overlay, name_ss.str().c_str(), 0);
        

// Write out mask as a png
//        std::stringstream mname_ss;
//        mname_ss << this->bgr->getId() << "_mask.png";
//        ImagenomicAnalytics::IO::writeImage<ImageType>(itkMask, mname_ss.str().c_str(), 0);
    }
	else
		std::cout <<"Segmentation: input data NULL"<< std::endl;


    //imwrite ("/lustre/atlas/proj-shared/csc143/lot/u24/test/DB-rt-adios/masktest.png", outMask);


	this->mask->setData(outMask);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}
