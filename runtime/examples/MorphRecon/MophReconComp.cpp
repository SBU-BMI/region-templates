
#include "MophReconComp.h"

MophReconComp::MophReconComp() {
	markerImage = new Mat();
	maskImage = new Mat();
	setComponentName("MophReconComp");
}

MophReconComp::~MophReconComp() {
	t2 = cciutils::ClockGetTime();
	std::cout << "~MorphRecon  took "<< t2-t1<< " ms" <<std::endl;
	delete markerImage;
	delete maskImage;
}
cv::Mat buildWholeSlide(std::string folderImageName){
 	cv::Mat outputImage;
	std::vector<std::string> filenames;
	std::vector<std::string> seg_output;
	ReadInputFileNames::getFiles(folderImageName, "./", filenames, seg_output);

	std::cout << "fileNames.size():" <<filenames.size() << std::endl;
	if(filenames.size() == 0){
		outputImage = imread(folderImageName, -1);
	}else{
		int x, y;
		ReadInputFileNames::getDimensions(x, y, filenames);
		x+=4096;
		y+=4096;

		if(x > y){
			y=x;
		}else{
			x=y;
		}
		std::cout << "x="<<x<<"y="<<y<<std::endl;
		outputImage = Mat::zeros(x,y,CV_8UC3);

		ReadInputFileNames::assembleTiles(outputImage, filenames);


//		imwrite("outputBig.tiff", outputImage);
//		imwrite("outputBig.tif", outputImage);
//		imwrite("outputBig.jp2", outputImage);
		imwrite("outputBig.png", outputImage);


		std::cout << "output.rows:"<<outputImage.rows <<std::endl;

//		Mat test = imread("outputBig.ppm", -1);
//		std::cout << "test.rows:"<< test.rows <<std::endl;
//		Mat res = outputImage - test;
//		   vector<Mat> planes;
//    			split(res, planes);
//		for(int i = 0; i < planes.size(); i++){
//			std::cout << "Non-zero = " << cv::countNonZero(planes[i]) << std::endl;
//		}
	}

	if(!outputImage.data){
		std::cout << "failed to read image/folder:"<< folderImageName <<std::endl;
		exit(1);
	}

	return outputImage;
}

int MophReconComp::run()
{
//	Mat test = imread("outputBig2.tif", -1);
//	if(test.data == NULL){
//		std::cout << "Reading failed" << std::endl;
//	}else{
//		std::cout << "Rows:"<< test.rows << std::endl;
//	}
	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;

	std::string markerImageName = ((ArgumentString*)this->getArgument(0))->getArgValue();
	std::string maskImageName = ((ArgumentString*)this->getArgument(1))->getArgValue();
	std::string tileSizeS = ((ArgumentString*)this->getArgument(2))->getArgValue();
	int tileSize = atoi(tileSizeS.c_str());

//	(*markerImage) = buildWholeSlide(markerImageName);
//	(*maskImage) = buildWholeSlide(maskImageName);

//	Mat marker1 = imread(markerImageName, -1);
//	Mat mask1 = imread(maskImageName, -1);
//
//	Mat recon1 = nscale::imreconstruct<unsigned char>(marker1, mask1, 8);
//	imwrite("out-recon8.ppm", recon1);
//

	int zoomFactor = 16;

	Mat marker = cv::imread(markerImageName, -1);
	Mat mask = cv::imread(maskImageName, -1);

//	if(zoomFactor > 1){
//		Mat tempMarker(marker.cols*zoomFactor ,marker.rows*zoomFactor, marker.type());
//		Mat tempMask(mask.cols*zoomFactor ,mask.rows*zoomFactor, mask.type());
//		for(int x = 0; x < zoomFactor; x++){
//			for(int y = 0; y <zoomFactor; y++){
//				Mat roi(tempMarker, cv::Rect(marker.cols*x, marker.rows*y, marker.cols, marker.rows));
//				marker.copyTo(roi);
//				Mat roiMask(tempMask, cv::Rect(mask.cols*x, mask.rows*y, mask.cols, mask.rows ));
//				mask.copyTo(roiMask);
//			}
//		}
//		marker = tempMarker;
//		mask = tempMask;
//	}
//

	if (!marker.data) {
		std::cout << "ERROR: invalid image: "<< markerImageName <<" or: "<< maskImageName << std::endl;
		return false;
	}else{
		std::cout << "Yep. Image was successfully load: "<< markerImageName << " and "<< maskImageName << std::endl;
	}
	if(zoomFactor > 1){
		Mat tempMarker = Mat::zeros((marker.cols*zoomFactor)+2,(marker.rows*zoomFactor)+2, marker.type());
		Mat tempMask = Mat::zeros((mask.cols*zoomFactor)+2 ,(mask.rows*zoomFactor)+2, mask.type());
		for(int x = 0; x < zoomFactor; x++){
			for(int y = 0; y <zoomFactor; y++){
				Mat roi(tempMarker, cv::Rect((marker.cols*x)+1, marker.rows*y+1, marker.cols, marker.rows));
				marker.copyTo(roi);
				Mat roiMask(tempMask, cv::Rect((mask.cols*x)+1, mask.rows*y+1, mask.cols, mask.rows ));
				mask.copyTo(roiMask);
			}
		}
		marker = tempMarker;
		mask = tempMask;
	}

	
	(*markerImage) = marker;
	(*maskImage) = mask;


//	t1 = cciutils::ClockGetTime();
//	Mat recon1 = nscale::imreconstruct<unsigned char>(marker, mask, 8);
//	t2 = cciutils::ClockGetTime();
//	std::cout << "Sequential took "<< t2-t1<< " ms"<<std::endl;

	int nTilesX=((*markerImage).cols-2)/tileSize;
	int nTilesY=((*markerImage).rows-2)/tileSize;

	Mat *markerCopy = new Mat();
//	(*markerImage).copyTo((*markerCopy));

	std::cout << "img.cols="<< (*markerImage).cols<<std::endl;
	std::cout << "nTilesX="<<nTilesX<< " nTilesY="<<nTilesY<<" tileSize="<<tileSize <<std::endl;
	TaskFixRecon *taskFixRecon =  new TaskFixRecon(markerImage, markerCopy, maskImage, tileSize);
	t1 = cciutils::ClockGetTime();

	for(int tileY=0; tileY < nTilesY; tileY++){
		for(int tileX=0; tileX < nTilesX; tileX++){
			TaskTileRecon *taskTileRecon = new TaskTileRecon(markerImage, maskImage, tileX, tileY, tileSize);
			taskFixRecon->addDependency(taskTileRecon);
			this->executeTask(taskTileRecon);
		}
	}

	this->executeTask(taskFixRecon);


	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactorySeg() {
	return new MophReconComp();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister("MophReconComp", &componentFactorySeg);


