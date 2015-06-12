
#include "FileSystemImageDataSource.h"

FileSystemImageDataSource::FileSystemImageDataSource() {
}

FileSystemImageDataSource::FileSystemImageDataSource(std::string fileName) {
	this->setType(DataSourceType::FILE_SYSTEM);
	this->setName(fileName);
}
FileSystemImageDataSource::~FileSystemImageDataSource() {
	std::cout << "~FileSystemImageDataSource()"<< std::endl;
}

bool FileSystemImageDataSource::instantiateRegion(DataRegion *dataRegionParam) {
	DenseDataRegion2D* dataRegion = dynamic_cast<DenseDataRegion2D*>(dataRegionParam);

	cv::Mat chunkData = cv::imread(this->getName(), -1);
	if(chunkData.empty()){
		std::cout << "Failed to read image:" << this->getName() << std::endl;
		exit(1);
	}

	BoundingBox ROIBB (Point(0, 0, 0), Point(chunkData.cols-1, chunkData.rows-1, 0));
	dataRegion->setData(chunkData);
	dataRegion->setBb(ROIBB);
	dataRegion->setType(chunkData.type());

	return true;
}

bool FileSystemImageDataSource::writeRegion(DataRegion *dataRegion) {

}

void FileSystemImageDataSource::print() {
	std::cout << "FileSystemImageDataSource:"<< this->getName() << std::endl;

}
