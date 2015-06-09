/*
 * ImageFileChunksDataSource.cpp
 *
 *  Created on: Oct 24, 2012
 *      Author: george
 */

#include "ImageFileChunksDataSource.h"

ImageFileChunksDataSource::ImageFileChunksDataSource() {

}

vector<string> ImageFileChunksDataSource::tokenize(const std::string & str, const std::string & delim)
{
	vector<string> tokens;
	size_t p0 = 0, p1 = string::npos;

	while(p0 != string::npos)
	{
		p1 = str.find_first_of(delim, p0);
		if(p1 != p0)
		{
			string token = str.substr(p0, p1 - p0);
			tokens.push_back(token);
		}
		p0 = str.find_first_not_of(delim, p1);
	}
	return tokens;

}

ImageFileChunksDataSource::ImageFileChunksDataSource(std::string folderPath, std::string extension, int tileSize, std::string outFolder) {
	this->setTileSize(tileSize);
	this->folderName = folderPath;
	this->extension = extension;
	this->outFolder = outFolder;

	FileUtils fileUtils(extension);
	if(outFolder.size() > 0){
		bool retMkdir = fileUtils.mkdirs(outFolder);
		if(retMkdir != true){
			std::cout << "Failed to created output dir: "<< outFolder << std::endl;
		}
	}


	std::vector<std::string> fileList;
	fileUtils.getFilesInDirectory(folderPath, fileList);

	for(int i = 0; i < fileList.size(); i++){
		const std::string a, b;
		std::vector<std::string> tokens = this->tokenize(fileList[i], "-/");
		if(tokens.size() > 1){
			std::string subToken = tokens[tokens.size()-1].substr(0, tokens[tokens.size()-1].size()-extension.size());
			int x = atoi(tokens[tokens.size()-2].c_str());
			int y = atoi(subToken.c_str());
			int z = 0;

	#ifdef DEBUG
			std::cout <<tokens[tokens.size()-2].c_str() <<std::endl;
			std::cout << "x:"<<x<<" y:"<< y << " file:"<< fileList[i] << " tokeSize:" << tokens.size()<< " value:"<< tokens[tokens.size()-2]<<std::endl;
#endif
			Point lb(x, y, z);
			Point ub(x+tileSize-1, y+tileSize-1, z);
			BoundingBox tileBB(lb, ub);
			this->dataFilePaths.push_back(std::make_pair(tileBB,fileList[i]));
		}else{
			std::cout << "Error parsing file name"<< std::endl;
		//	exit(1);
		}
	}
}

ImageFileChunksDataSource::~ImageFileChunksDataSource() {
	std::cout << "~ImageFileChunksDataSource()"<<std::endl;
}

int ImageFileChunksDataSource::getTileSize() const {
	return tileSize;
}

bool ImageFileChunksDataSource::instantiateRegion(DataRegion *dataRegionParam) {
	//DenseDataRegion2D * data = new DenseDataRegion2D();
	// data->allocData(regionBB.sizeCoordX(), regionBB.sizeCoordY(), regionBB.sizeCoordZ());
	// Create data structure, and initialize it
	// Find bounding boxes involved.
	// 	For each involved bounding box
	//		Read data and copy intersection area
	DenseDataRegion2D* dataRegion = dynamic_cast<DenseDataRegion2D*>(dataRegionParam);
	BoundingBox ROIBB = dataRegion->getBb();
	cv::Mat ROI;

	int elementsDataType = -1;
	for(int i = 0; i < this->dataFilePaths.size(); i++){
		pair<BoundingBox,std::string> chunkInfo = dataFilePaths[i];
		// if current chunk is within the region of interest, read it.
		if(chunkInfo.first.doesIntersect(ROIBB)){
			cv::Mat chunkData = cv::imread(chunkInfo.second, -1);
			if(chunkData.empty()){
				std::cout << "Failed to read image:" << chunkInfo.second << std::endl;
				exit(1);
			}
			// First data chunk found
			if(elementsDataType == -1){
				elementsDataType = chunkData.type();
				//data->allocData(ROIBB.sizeCoordX(), ROIBB.sizeCoordY(), elementsDataType);
				ROI.create(ROIBB.sizeCoordX(), ROIBB.sizeCoordY(), elementsDataType);
			}
			// Calculate intersection between the current data chunk domain with the ROI domain
			BoundingBox intersection = chunkInfo.first.intersection(ROIBB);

			// Calculate the place where the intersection should be written within the ROI.
			cv::Rect intersectionInROI(intersection.getLb().getX() - ROIBB.getLb().getX(), intersection.getLb().getY()- ROIBB.getLb().getY(), intersection.sizeCoordX(), intersection.sizeCoordY() );
			cv::Mat ROIIntersectionHeader(ROI, intersectionInROI);

			// Calculate place where the intersection is with current chunk data
			cv::Rect intersectionInChunk(intersection.getLb().getX() - chunkInfo.first.getLb().getX(), intersection.getLb().getY()- chunkInfo.first.getLb().getY(), intersection.sizeCoordX(), intersection.sizeCoordY() );
			cv::Mat chunkIntersectionHeader(chunkData, intersectionInChunk);

			chunkIntersectionHeader.copyTo(ROIIntersectionHeader);
			chunkData.release();
		}
	}
	dataRegion->setData(ROI);
//	data->setName(this->folderName);


	return true;

}

bool ImageFileChunksDataSource::writeRegion(DataRegion *dataRegion) {

	std::string outfileName = this->outFolder + "/";
	outfileName.append(dataRegion->getName());
	outfileName.append(".");
	outfileName.append(this->extension);
	assert(dataRegion->getType() == RegionTemplateType::DENSE_REGION_2D);


	DenseDataRegion2D *ddr2D = dynamic_cast<DenseDataRegion2D*>(dataRegion);

	std::cout << "OutfileName:"<< outfileName << std::endl;

	cv::imwrite(outfileName, ddr2D->getData());
}

void ImageFileChunksDataSource::setTileSize(int tileSize) {
	this->tileSize = tileSize;
}

void ImageFileChunksDataSource::print() {
	std::cout << "BEGIN: Printing ImageFileChunksDataSource content" << std::endl;
	for(int i = 0; i < this->dataFilePaths.size(); i++){
		pair<BoundingBox,std::string> chunkInfo = dataFilePaths[i];
		chunkInfo.first.print();
		std::cout << chunkInfo.second<< std::endl;
	}
	std::cout << "END: Printing ImageFileChunksDataSource content" << std::endl;
}
