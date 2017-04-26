#include "TaskFeatures.h"

#include "MultipleObjectFeatureAnalysisFilter.h"
#include <itkOpenCVImageBridge.h>

TaskFeatures::TaskFeatures(DenseDataRegion2D* bgr, DenseDataRegion2D* mask, DataRegion2DUnaligned* features) {
	this->bgr = bgr;
	this->mask = mask;
	this->features = features;
}

TaskFeatures::~TaskFeatures() {
	if(bgr != NULL) delete bgr;
	if(mask != NULL) delete mask;
}

bool TaskFeatures::run(int procType, int tid) {
	int *bbox = NULL;
	int compcount;
	uint64_t t1 = Util::ClockGetTimeProfile();

	cv::Mat inputImage = this->bgr->getData();
	cv::Mat maskImage = this->mask->getData();
	cv::Mat maskImageBinary;
	std::cout << "nChannels:  "<< maskImage.channels() << std::endl;

	ImagenomicAnalytics::MultipleObjectFeatureAnalysisFilter featureAnalyzer;
	if(inputImage.rows > 0 && maskImage.rows > 0 && cv::countNonZero (maskImage) > 0){

        cv::threshold (maskImage, maskImageBinary, 0.5, 1, cv::THRESH_BINARY);

		//nscale::ObjFeatures::calcFeatures(inputImage, maskImage);
		std::cout << "FEATURE COMPUTATION COMES HERE!!" << std::endl;
		featureAnalyzer.setInputRGBImage(itk::OpenCVImageBridge::CVMatToITKImage<itkRGBImageType>(inputImage));
		featureAnalyzer.setObjectBinaryMask(itk::OpenCVImageBridge::CVMatToITKImage<itkBinaryMaskImageType>(maskImageBinary));
		featureAnalyzer.setTopLeft(0,0);
		featureAnalyzer.update();

		std::vector< std::vector<FeatureValueType> > features = featureAnalyzer.getFeatures();
		this->features->setData(features);
        featureAnalyzer.setFeatureNames();
        this->features->labels = featureAnalyzer.getFeatureNames();
std::cout << "Setting " << this->features->labels.size() << " labels in TaskFeatures" << std::endl;

/*		// TESTING data region write/read to/from FS
		DataRegionFactory::writeDDR2DFS(this->features, "./", false);
		DataRegion*dr= new DataRegion();
		dr->setType(this->features->getType());
		dr->setVersion(this->features->getVersion());
		dr->setName(this->features->getName());
		dr->setId(this->features->getId());
		dr->setTimestamp(this->features->getTimestamp());
		DataRegionFactory::readDDR2DFS(&dr, -1, "./", false);
		DataRegion2DUnaligned* features2 = dynamic_cast<DataRegion2DUnaligned*>(dr);
		if(this->features->compare(features2)){
			std::cout << "EQUAL" << std::endl;
		}else{
			std::cout << "Features DIFFER: " << std::endl;
		}
		delete features2;*/

	}else{
        //Setup empty feature output
        featureAnalyzer.setFeatureNames();
        this->features->labels = featureAnalyzer.getFeatureNames();
        std::vector< std::vector<FeatureValueType> > features (0);
        this->features->setData(features);
	}

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Feature Computation time elapsed: "<< t2-t1 << std::endl;
}
