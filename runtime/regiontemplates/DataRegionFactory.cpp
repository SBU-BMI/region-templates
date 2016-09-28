/*
 * DataRegionFactory.cpp
 *
 *  Created on: Feb 15, 2013
 *      Author: george
 */

#include "DataRegionFactory.h"
#include "itkImage.h"
#include "itkRGBPixel.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkOpenCVImageBridge.h"
#include "adios.h"
#include "ItkAdiosIOFactory.h"

std::string number2String(int x){
	std::ostringstream ss;
	ss<<x;
	return ss.str();
}

DataRegionFactory::DataRegionFactory() {
}

bool DataRegionFactory::createLockFile(DataRegion* dr, std::string path) {
	std::string lockFileName = createOutputFileName(dr, path, "");
	// create lock file name
	lockFileName.append(".loc");
	FILE *pFile = fopen(lockFileName.c_str(), "w");

	if(pFile == NULL){
#ifdef DEBUG
		std::cout << "ERROR:::: could not write lock file" << std::endl;
#endif
		return false;
	}
	int drType = dr->getType();
	// write out RT type to lock file
	fwrite(&drType, sizeof(int), 1, pFile);
	fclose(pFile);
	return true;
}


int DataRegionFactory::lockFileExists(DataRegion* dr, std::string path) {
	int retValue = -1;

	std::string lockFileName = createOutputFileName(dr, path, "");
	// create lock file name
	lockFileName.append(".loc");

	FILE *pFile = fopen(lockFileName.c_str(), "r");
	if(pFile == NULL){
#ifdef DEBUG
		std::cout << "ERROR: lock file: "<< lockFile << std::endl;
#endif
	}else{
		// Read region template type
		int read = fread(&retValue, sizeof(int), 1, pFile);
		assert(read = sizeof(int));
		fclose(pFile);
	}

	return retValue;
}

std::string DataRegionFactory::createOutputFileName(DataRegion* dr, std::string path, std::string extension) {
	std::string outputFileName;

	if(!path.empty())outputFileName.append(path);

	outputFileName.append(dr->getName());
	outputFileName.append("-").append(dr->getId());
	outputFileName.append("-").append(number2String(dr->getVersion()));
	outputFileName.append("-").append(number2String(dr->getTimestamp()));
	outputFileName.append(extension);

	return outputFileName;
}

bool DataRegionFactory::writeDr2DUn(DataRegion2DUnaligned* dr, std::string outputFile) {
	// create file to write data region data.
	FILE *pFile = fopen(outputFile.c_str(), "wb+");
	if(pFile == NULL){
		std::cout << "Error trying to create file: "<< outputFile << std::endl;
		exit(1);
	}
	int numVecs = dr->data.size();

	// write number of vectors that will be staged
	int writtenSize = fwrite(&numVecs, sizeof(int), 1, pFile);
	assert(writtenSize ==  sizeof(int));

	for(int i = 0; i < numVecs; i++){
		int size = dr->data[i].size();
		// write number of elements in current vector
		writtenSize = fwrite(&size, sizeof(int), 1, pFile);
		assert(writtenSize ==  sizeof(int));

		// write vector elements
		int writtenSize2 = fwrite((dr->data[i].data()), sizeof(double), size, pFile);
		assert(writtenSize2 ==  sizeof(double) * size);

	}
	fclose(pFile);

	return true;
}

bool DataRegionFactory::readDr2DUn(DataRegion2DUnaligned* dr, std::string inputFile) {
	// open file to read data region data.
	FILE *pFile = fopen(inputFile.c_str(), "rb");
	if(pFile == NULL){
		std::cout << "Error trying to open file: "<< inputFile << std::endl;
		exit(1);
	}
	// read number of vectors
	int numVecs = 0;
	int readSize = fread(&numVecs, sizeof(int), 1, pFile);
	dr->data.resize(numVecs);
	assert(readSize == sizeof(int));

	for(int i = 0; i < numVecs; i++){
		// read number of elements in this vector
		int numberOfElements = 0;
		readSize = fread(&numberOfElements, sizeof(int), 1, pFile);
		dr->data[i].resize(numberOfElements);
		assert(readSize == sizeof(int));

		// read actual vector elements
		int readSize2 = fread(dr->data[i].data(), sizeof(double), numberOfElements, pFile);
		assert(readSize == sizeof(double)*numberOfElements);

	}
	fclose(pFile);
	return true;
}


DataRegionFactory::~DataRegionFactory() {

}



bool DataRegionFactory::readDDR2ADIOS(DataRegion **dataRegion, int chunkId, std::string path){
	int drType = -1;
	// it is application input, the type is provide when user creates the
	// data region. Otherwise, a .loc file exists and it stores the data region type
	if((*dataRegion)->getIsAppInput() == true){
		drType = (*dataRegion)->getType();
	}else{
		// returns -1 if lock file is not found
		drType = DataRegionFactory::lockFileExists((*dataRegion), path);
	}

	if(drType == -1) return false; // means not an input and no lock file found


	switch(drType){
	case DataRegionType::DENSE_REGION_2D:{
		DenseDataRegion2D* dr2D = new DenseDataRegion2D();//dynamic_cast<DenseDataRegion2D*>(dataRegion);
		dr2D->setName((*dataRegion)->getName());
		dr2D->setId((*dataRegion)->getId());
		dr2D->setTimestamp((*dataRegion)->getTimestamp());
		dr2D->setVersion((*dataRegion)->getVersion());
		dr2D->setIsAppInput((*dataRegion)->getIsAppInput());
		dr2D->setInputFileName((*dataRegion)->getInputFileName());

		cv::Mat chunkData;
#ifdef DEBUG
		std::cout << "readDDR2DFS: dataRegion: "<< dr2D->getName()<< " id: "<< dr2D->getId()<< " version:" << dr2D->getVersion() <<" outputExt: "<< dr2D->getOutputExtension() << std::endl;
#endif
		if(dr2D->getOutputExtension() == DataRegion::XML){
			// if it is an Mat stored as a XML file
			std::string inputFile;

			if(dr2D->getIsAppInput()){
				inputFile = dr2D->getInputFileName();
			}else{
				inputFile = createOutputFileName(dr2D, path, ".xml");
			}
			cv::FileStorage fs2(inputFile, cv::FileStorage::READ);
			if(fs2.isOpened()){
				fs2["mat"] >> chunkData;
				fs2.release();
#ifdef DEBUG
				std::cout << "LOADING XML input data: " << inputFile << " dataRegion: "<< dataRegion->getName() << " chunk.rows: "<< chunkData.rows<< " chunk.cols: "<< chunkData.cols<<std::endl;
#endif
			}else{
#ifdef DEBUG
				std::cout << "Failed to read Data region. Failed to open FILE: " << inputFile << std::endl;
#endif
				exit(1);
			}
		}else{
			if(dr2D->getOutputExtension() == DataRegion::PBM){
				std::string inputFile;

				if(dr2D->getIsAppInput()){
					inputFile = dr2D->getInputFileName();
				}else{
					inputFile = createOutputFileName(dr2D, path, ".bp");
				}

				//chunkData = cv::imread(inputFile, -1); // read image

            	itk::ItkAdiosIOFactory::RegisterOneFactory();

            	typedef itk::RGBPixel<unsigned char> RGBPixelType;
            	typedef itk::Image<RGBPixelType, 2> itkRGBImageType;

            	typedef itk::ImageFileReader<itkRGBImageType> ReaderType;

            	typename ReaderType::Pointer reader = ReaderType::New();
            	reader->SetFileName (inputFile);

            	chunkData = itk::OpenCVImageBridge::ITKImageToCVMat<itkRGBImageType>(reader->GetOutput());

				if(chunkData.empty()){
					std::cout << "Failed to read image in readDDR2ADIOS" << inputFile << std::endl;
				}else{
					BoundingBox ROIBB (Point(0, 0, 0), Point(chunkData.cols-1, chunkData.rows-1, 0));
					dr2D->setData(chunkData);
					dr2D->setBb(ROIBB);
				}
			}
		}
		// delete father class instance and attribute the specific data region read
		delete (*dataRegion);
		(*dataRegion) = (DataRegion*)dr2D;
		break;
	}
	case DataRegionType::REGION_2D_UNALIGNED:{
		DataRegion2DUnaligned* dr2DUn = new DataRegion2DUnaligned();//dynamic_cast<DataRegion2DUnaligned*>(dataRegion);
		dr2DUn->setName((*dataRegion)->getName());
		dr2DUn->setId((*dataRegion)->getId());
		dr2DUn->setTimestamp((*dataRegion)->getTimestamp());
		dr2DUn->setVersion((*dataRegion)->getVersion());

		std::string inputFileName = createOutputFileName(dr2DUn, path, ".vec");

		// TODO: ADIOS this
		readDr2DUn(dr2DUn, inputFileName);

		// delete data region passed as a parameter and replace it with the actual data region with the data
		delete (*dataRegion);
		(*dataRegion) = (DataRegion*)dr2DUn;

		break;
	}
	default:
		std::cout << "readDDR2ADIOS: ERROR: Unknown Region template type: "<< drType <<std::endl;
		exit(1);
		break;
	}
	return true;
}
bool DataRegionFactory::readDDR2DFS(DataRegion **dataRegion, int chunkId, std::string path, bool ssd){
	int drType = -1;
	// it is application input, the type is provide when user creates the
	// data region. Otherwise, a .loc file exists and it stores the data region type
	if((*dataRegion)->getIsAppInput() == true){
		drType = (*dataRegion)->getType();
	}else{
		// returns -1 if lock file is not found
		drType = DataRegionFactory::lockFileExists((*dataRegion), path);
	}

	if(drType == -1) return false; // means not an input and no lock file found


	switch(drType){
	case DataRegionType::DENSE_REGION_2D:{
		DenseDataRegion2D* dr2D = new DenseDataRegion2D();//dynamic_cast<DenseDataRegion2D*>(dataRegion);
		dr2D->setName((*dataRegion)->getName());
		dr2D->setId((*dataRegion)->getId());
		dr2D->setTimestamp((*dataRegion)->getTimestamp());
		dr2D->setVersion((*dataRegion)->getVersion());
		dr2D->setIsAppInput((*dataRegion)->getIsAppInput());
		dr2D->setInputFileName((*dataRegion)->getInputFileName());

		cv::Mat chunkData;
#ifdef DEBUG
		std::cout << "readDDR2DFS: dataRegion: "<< dr2D->getName()<< " id: "<< dr2D->getId()<< " version:" << dr2D->getVersion() <<" outputExt: "<< dr2D->getOutputExtension() << std::endl;
#endif
		if(dr2D->getOutputExtension() == DataRegion::XML){
			// if it is an Mat stored as a XML file
			std::string inputFile;

			if(dr2D->getIsAppInput()){
				inputFile = dr2D->getInputFileName();
			}else{
				inputFile = createOutputFileName(dr2D, path, ".xml");
			}
			cv::FileStorage fs2(inputFile, cv::FileStorage::READ);
			if(fs2.isOpened()){
				fs2["mat"] >> chunkData;
				fs2.release();
#ifdef DEBUG
				std::cout << "LOADING XML input data: " << inputFile << " dataRegion: "<< dataRegion->getName() << " chunk.rows: "<< chunkData.rows<< " chunk.cols: "<< chunkData.cols<<std::endl;
#endif
			}else{
#ifdef DEBUG
				std::cout << "Failed to read Data region. Failed to open FILE: " << inputFile << std::endl;
#endif
				exit(1);
			}
		}else{
			if(dr2D->getOutputExtension() == DataRegion::PBM){
				std::string inputFile;

				if(dr2D->getIsAppInput()){
					inputFile = dr2D->getInputFileName();
				}else{
					if(ssd){
						inputFile = createOutputFileName(dr2D, path, ".pbm");
					}else{
						inputFile = createOutputFileName(dr2D, path, ".tiff");
					}
				}
				chunkData = cv::imread(inputFile, -1); // read image
				if(chunkData.empty()){
					std::cout << "Failed to read image:" << inputFile << std::endl;
				}else{
					BoundingBox ROIBB (Point(0, 0, 0), Point(chunkData.cols-1, chunkData.rows-1, 0));
					dr2D->setData(chunkData);
					dr2D->setBb(ROIBB);
				}
			}
		}
		// delete father class instance and attribute the specific data region read
		delete (*dataRegion);
		(*dataRegion) = (DataRegion*)dr2D;
		break;
	}
	case DataRegionType::REGION_2D_UNALIGNED:{
		DataRegion2DUnaligned* dr2DUn = new DataRegion2DUnaligned();//dynamic_cast<DataRegion2DUnaligned*>(dataRegion);
		dr2DUn->setName((*dataRegion)->getName());
		dr2DUn->setId((*dataRegion)->getId());
		dr2DUn->setTimestamp((*dataRegion)->getTimestamp());
		dr2DUn->setVersion((*dataRegion)->getVersion());

		std::string inputFileName = createOutputFileName(dr2DUn, path, ".vec");

		readDr2DUn(dr2DUn, inputFileName);

		// delete data region passed as a parameter and replace it with the actual data region with the data
		delete (*dataRegion);
		(*dataRegion) = (DataRegion*)dr2DUn;

		break;
	}
	default:
		std::cout << "readDDR2DFS: ERROR: Unknown Region template type: "<< drType <<std::endl;
		exit(1);
		break;
	}
	return true;
}

bool DataRegionFactory::writeDDR2ADIOS(DataRegion* dataRegion, std::string path) {
	bool retVal = true;

#ifdef DEBUG
		std::cout << "DataRegion: "<< dataRegion->getName() << " extension: "<< dataRegion->getOutputExtension() << std::endl;
#endif

		switch(dataRegion->getType()){
			case DataRegionType::DENSE_REGION_2D:{
				DenseDataRegion2D* dr2D = dynamic_cast<DenseDataRegion2D*>(dataRegion);
				if(dr2D->getOutputExtension() == DataRegion::PBM){
					std::string outputFile;

					outputFile = createOutputFileName(dr2D, path, ".bp");

#ifdef DEBUG
					std::cout << "rows: "<< dr2D->getData().rows << " cols: "<< dr2D->getData().cols <<std::endl;
#endif
					// check if there is any data to be written
					if(dr2D->getData().rows > 0 && dr2D->getData().cols > 0){

						//retVal = cv::imwrite(outputFile, dr2D->getData());

						itk::ItkAdiosIOFactory::RegisterOneFactory();

                		typedef itk::RGBPixel<unsigned char> RGBPixelType;
                		typedef itk::Image<RGBPixelType, 2> itkRGBImageType;
                		typedef itk::ImageFileWriter<itkRGBImageType> WriterType;

                		typename WriterType::Pointer writer = WriterType::New();
                		writer->SetFileName(outputFile);
						writer->SetInput(itk::OpenCVImageBridge::CVMatToITKImage<itkRGBImageType>(dr2D->getData()));
						try
                		{
                    		printf("PRE-UPDATE\n");
                    		writer->Update();
                    		printf("POST-UPDATE\n");
                		}
                		catch ( itk::ExceptionObject &err)
                		{
                		    std::cout << "Problem writing image." << std::endl;
                		    std::cout << err << std::endl;
							retVal = false;
                		}


					}

					createLockFile(dr2D, path);

				}else{
					if(dr2D->getOutputExtension() == DataRegion::XML){
						std::string outputFile;

						outputFile = createOutputFileName(dr2D, path, ".xml");

						cv::FileStorage fs(outputFile, cv::FileStorage::WRITE);
						// check if file has been opened correctly and write data
						if(fs.isOpened()){
							fs << "mat" << dr2D->getData();
							fs.release();
						}else{
							retVal =false;
						}
						// create lock.
						createLockFile(dr2D, path);

					}else{
						std::cout << "UNKNOWN file extension: "<< dataRegion->getOutputExtension() << std::endl;
					}
				}
				break;
			}
			case DataRegionType::REGION_2D_UNALIGNED:{
				DataRegion2DUnaligned* dr2DUn = dynamic_cast<DataRegion2DUnaligned*>(dataRegion);

				std::string outputFile;
				outputFile = createOutputFileName(dr2DUn, path, ".vec");

				// TODO: ADIOS this
				writeDr2DUn(dr2DUn, outputFile);

				createLockFile(dr2DUn, path);
				break;
			}
			default:
				std::cout << "writeDDR2DFS: ERROR: Unknown Region template type: "<< dataRegion->getType() <<std::endl;
				exit(1);
				break;
		}
		return retVal;
}

bool DataRegionFactory::readDDR2DATASPACES(DenseDataRegion2D *dataRegion) {
	int getReturn = 0;
#ifdef	WITH_DATA_SPACES

	std::cout << "ReadDDRDATASPACES: "<< dataRegion->getName() << std::endl; 
	
	if(dataRegion->getName().compare("BGR") == 0){
		cv::Mat dataChunk(4096, 4096, CV_8UC3, 0.0);
		char *matrix = dataChunk.ptr<char>(0);
        std::cout << "DATASPACES get BGR: "<< dataRegion->getId() << std::endl;
//                begin = Util::ClockGetTime();
                getReturn = dspaces_get(dataRegion->getId().c_str(), 1, sizeof(char), 0, 0, 0, (4096*3)-1, 4095, 0, matrix);
//                end = Util::ClockGetTime();
		if(getReturn < 0) dataChunk.release();
                std::cout << "DataSpaces Get return: "<< getReturn << std::endl;
		dataRegion->setData(dataChunk);
	}

	if(dataRegion->getName().compare("mask") == 0){
		cv::Mat dataChunk(4096, 4096, CV_8U);
		char *matrix = dataChunk.ptr<char>(0);
                std::cout << "DATASPACES get mask: "<< dataRegion->getId() << std::endl;
//                begin = Util::ClockGetTime();
                getReturn = dspaces_get(dataRegion->getId().c_str(), 1, sizeof(char), 0, 0, 0, 4095, 4095, 0, matrix);
//                end = Util::ClockGetTime();
		if(getReturn < 0) dataChunk.release();
                std::cout << "DataSpaces Get return: "<< getReturn << std::endl;
		dataRegion->setData(dataChunk);
	}

#endif
	return true;
}
bool DataRegionFactory::writeDDR2DFS(DataRegion* dataRegion, std::string path, bool ssd) {
	bool retVal = true;

#ifdef DEBUG
		std::cout << "DataRegion: "<< dataRegion->getName() << " extension: "<< dataRegion->getOutputExtension() << std::endl;
#endif

		switch(dataRegion->getType()){
			case DataRegionType::DENSE_REGION_2D:{
				DenseDataRegion2D* dr2D = dynamic_cast<DenseDataRegion2D*>(dataRegion);
				if(dr2D->getOutputExtension() == DataRegion::PBM){
					std::string outputFile;

					if(ssd){
						outputFile = createOutputFileName(dr2D, path, ".pbm");
					}else{
						outputFile = createOutputFileName(dr2D, path, ".tiff");
					}

#ifdef DEBUG
					std::cout << "rows: "<< dr2D->getData().rows << " cols: "<< dr2D->getData().cols <<std::endl;
#endif
					// check if there is any data to be written
					if(dr2D->getData().rows > 0 && dr2D->getData().cols > 0){
						retVal = cv::imwrite(outputFile, dr2D->getData());
					}

					createLockFile(dr2D, path);

				}else{
					if(dr2D->getOutputExtension() == DataRegion::XML){
						std::string outputFile;

						outputFile = createOutputFileName(dr2D, path, ".xml");

						cv::FileStorage fs(outputFile, cv::FileStorage::WRITE);
						// check if file has been opened correctly and write data
						if(fs.isOpened()){
							fs << "mat" << dr2D->getData();
							fs.release();
						}else{
							retVal =false;
						}
						// create lock.
						createLockFile(dr2D, path);

					}else{
						std::cout << "UNKNOWN file extension: "<< dataRegion->getOutputExtension() << std::endl;
					}
				}
				break;
			}
			case DataRegionType::REGION_2D_UNALIGNED:{
				DataRegion2DUnaligned* dr2DUn = dynamic_cast<DataRegion2DUnaligned*>(dataRegion);

				std::string outputFile;
				outputFile = createOutputFileName(dr2DUn, path, ".vec");

				writeDr2DUn(dr2DUn, outputFile);

				createLockFile(dr2DUn, path);
				break;
			}
			default:
				std::cout << "writeDDR2DFS: ERROR: Unknown Region template type: "<< dataRegion->getType() <<std::endl;
				exit(1);
				break;
		}
		return retVal;
}




bool DataRegionFactory::writeDDR2DATASPACES(DenseDataRegion2D* dataRegion) {
#ifdef WITH_DATA_SPACES
	std::cout << "Writing data region do DataSpaces: "<< dataRegion->getName() << std::endl;

//	cv::imwrite(dataRegion->getId(), dataRegion->getData());
	if(dataRegion->getData().cols > 0 && dataRegion->getData().rows > 0){
		char *matrix = dataRegion->getData().ptr<char>(0);

		std::cout << "dataSpaces put, dataregion: "<< dataRegion->getId() << std::endl;

		dspaces_put(dataRegion->getId().c_str(), 1, sizeof(char), 0, 0, 0, (dataRegion->getData().cols * dataRegion->getData().channels())-1, dataRegion->getData().rows-1, 0, matrix);
		dspaces_put_sync();
	}else{
		std::cout << "Data region: "<< dataRegion->getName() << " is empty. Not writing to DS!" << std::endl;
	}

#endif
	return true;
}

