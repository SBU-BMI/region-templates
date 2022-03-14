/*
 * DenseDataRegion2D.cpp
 *
 *  Created on: Oct 22, 2012
 *      Author: george
 */

#include "DenseDataRegion2D.h"
#include <opencv/cv.hpp>

// #define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
// #include <boost/stacktrace.hpp>

// #define DEBUG

DenseDataRegion2D::DenseDataRegion2D() {
    this->setType(DataRegionType::DENSE_REGION_2D);
}

DenseDataRegion2D::~DenseDataRegion2D() {
    // release chunked data if there is anything cached
    std::map<BoundingBox, cv::Mat>::iterator it;
    for (it = this->chunkedDataCaching.begin();
         it != this->chunkedDataCaching.end(); it++) {
        it->second.release();
    }
#ifdef DEBUG
    std::cout << "DDR2D destructor:" << this->getName() << ":" << this->getId()
              << std::endl;
    std::cout << "ddrs size:" << this->dataCPU.rows << "x" << this->dataCPU.cols
              << std::endl;
    std::cout << boost::stacktrace::stacktrace() << std::endl;
#endif
    this->dataCPU.release();
}

void DenseDataRegion2D::allocData(int x, int y, int type, int device) {
    if (Environment::CPU == device) {
        this->dataCPU.create(x, y, type);
        (this->dataCPU) = cv::Scalar(0);
    } else if (Environment::GPU == device) {
#ifdef WITH_CUDA
        this->dataGPU.create(x, y, type);
#endif
    } else {
        std::cout << __FILE__ << ":" << __LINE__ << ": unknown type"
                  << std::endl;
    }
    this->cachedDataSize =
        this->dataCPU.rows * this->dataCPU.cols * this->dataCPU.elemSize();
}

void DenseDataRegion2D::releaseData(int device) {
    if (Environment::CPU == device) {
        this->dataCPU.release();
    } else if (Environment::GPU == device) {
#ifdef WITH_CUDA
        this->dataGPU.release();
#endif
    } else {
        std::cout << __FILE__ << ":" << __LINE__ << ": unknown type"
                  << std::endl;
    }
    this->cachedDataSize = 0;
}

void DenseDataRegion2D::setData(cv::Mat data) {
    this->dataCPU = data;
    this->cachedDataSize =
        this->dataCPU.rows * this->dataCPU.cols * this->dataCPU.elemSize();
}

cv::Mat DenseDataRegion2D::getData() { return this->dataCPU; }

cv::Mat *DenseDataRegion2D::getData(int x, int y, int width, int height,
                                    bool cacheData) {
    cv::Mat *ret = NULL;

    // create a bounding box w/ the area requested by the user
    BoundingBox userROI(Point(x, y, 0),
                        Point(x + width - 1, y + height - 1, 0));

    // if requested area is the default (x=-1 or y=-1), change it to be the data
    // region ROI
    if (x < 0 || y < 0) {
        userROI = this->getROI();
        std::cout << "ROI:";
        this->getROI().print();
        std::cout << std::endl;
    }

    // iterate over all the chunked data bounding boxes and instantiate those
    // intersecting the ROI requested by the end-user
    for (int i = 0; i < this->getBB2IdSize(); i++) {
        std::pair<BoundingBox, std::string> chunkInfo =
            this->getBB2IdElement(i);

        // check if BB intersects ROI
        if (userROI.doesIntersect(chunkInfo.first)) {
            // check if BB is already cached.
            std::map<BoundingBox, cv::Mat>::iterator it =
                this->chunkedDataCaching.find(chunkInfo.first);

            // if it was not in cache, load it to cache
            if (it == this->chunkedDataCaching.end()) {
                this->loadChunkToCache(i);
                // search it again to update iterator
                it = this->chunkedDataCaching.find(chunkInfo.first);
            } else {
#ifdef DEBUG
                std::cout << "Data chunk found in cache: ";
                chunkInfo.first.print();
                std::cout << std::endl;
#endif
            }

            // make sure that information is now cached
            if (it != this->chunkedDataCaching.end()) {
                cv::Mat currentChunk = it->second;

                // if this is the first time we're copying, alloc data
                if (ret == NULL) {
                    ret = new cv::Mat(
                        cv::Size(userROI.sizeCoordX(), userROI.sizeCoordY()),
                        currentChunk.type());
                }

                // Copy intersecting region to user data
                BoundingBox intersection =
                    userROI.intersection(chunkInfo.first);

                // set ROI in the current data chunk:
                int intersectionWidth = intersection.getUb().getX() -
                                        intersection.getLb().getX() + 1;
                int intersectionHeight = intersection.getUb().getY() -
                                         intersection.getLb().getY() + 1;

                // the cv::Mat start at (0,0) but the intersection is global, so
                // we have to shift the intersection by the ROI size
                cv::Rect curROI(intersection.getLb().getX() -
                                    chunkInfo.first.getLb().getX(),
                                intersection.getLb().getY() -
                                    chunkInfo.first.getLb().getY(),
                                intersectionWidth, intersectionHeight);
                // create a header w/ the ROI
                cv::Mat currentChunkROI = currentChunk(curROI);

                // create a ROI for the user-requested region, using the same
                // procedure
                cv::Rect retROI(
                    intersection.getLb().getX() - userROI.getLb().getX(),
                    intersection.getLb().getY() - userROI.getLb().getY(),
                    intersectionWidth, intersectionHeight);
                // create a header for the user data mat
                cv::Mat retDataROI = (*ret)(retROI);
                currentChunkROI.copyTo(retDataROI);

                // we're not caching data this time, delete loaded chunk from
                // cache
                if (cacheData == false) {
                    this->deleteChunkFromCache(chunkInfo.first);
                }
            } else {
                std::cout << "FAIL loading bounding box to cache:  BB ";
                chunkInfo.first.print();
                std::cout << std::endl;
            }
        }
    }

    return ret;
}

int DenseDataRegion2D::getXDimensionSize() { return this->dataCPU.cols; }

bool DenseDataRegion2D::loadChunkToCache(int chunkIndex) {
    cv::Mat data;
    // Get info about the Id of the file in which the data is stored
    std::pair<BoundingBox, std::string> data_pair =
        this->getBB2IdElement(chunkIndex);
    if (data_pair.second.size() > 0) {
        switch (this->getInputType()) {
            case DataSourceType::FILE_SYSTEM: {
                // read the data file
                data = cv::imread(data_pair.second, -1);
                if (data.empty()) {
                    std::cout << "Failed to read image:" << data_pair.second
                              << std::endl;
                } else {
                    // if it was successfully, insert data into the data region
                    // vector chunked data
                    this->insertChukedData(data_pair.first, data);
                    return true;
                }
            } break;

            case DataSourceType::FILE_SYSTEM_TEXT_FILE: {
                // read the data file
                std::ifstream infile(data_pair.second.c_str());
                int           columns, rows;
                int           a;
                infile >> columns >> rows;
                data = cv::Mat(rows, columns, CV_32S);

                // Read image from text file and find the bounding boxes
                for (int i = 0; i < rows; ++i) {
                    for (int j = 0; j < columns; ++j) {
                        infile >> a;
                        data.at<int>(i, j) = a;
                    }
                }
                if (data.empty()) {
                    std::cout << "Failed to read image:" << data_pair.second
                              << std::endl;

                } else {
                    // if it was successfully, insert data into the data region
                    // vector chunked data
                    this->insertChukedData(data_pair.first, data);
                    return true;
                }
            } break;

            default: {
                std::cout << "Unknown data source type:" << this->getInputType()
                          << std::endl;
            } break;
        }
    }
    return false;
}

void DenseDataRegion2D::deleteChunkFromCache(BoundingBox boundingBox) {
    std::map<BoundingBox, cv::Mat>::iterator it =
        this->chunkedDataCaching.find(boundingBox);
    if (it != this->chunkedDataCaching.end()) {
        it->second.release();
        this->chunkedDataCaching.erase(it);
    }
}

DataRegion *DenseDataRegion2D::clone(bool copyData) {
    DenseDataRegion2D *clonedDataRegion = new DenseDataRegion2D();

    // First, copy information stored into the DataRegion father's class.
    // This is done by serializing that class and deserializing it into
    // the "cloned" data region.

    // get size need to serialize fathers class
    int serializeSize = this->serializationSize();

    // create serialization buffer
    char *serBuffer = (char *)malloc(sizeof(char) * serializeSize);

    // serialize fathers class
    this->serialize(serBuffer);

    // deserialize information using the "clone object"
    clonedDataRegion->deserialize(serBuffer);
    // release buffer
    free(serBuffer);

    // create data clone.
    cv::Mat cloneData;

    // copy data as well if copyData is set
    if (copyData) {
        int   rows = this->dataCPU.rows;
        int   cols = this->dataCPU.cols;
        char *data = (char *)this->dataCPU.data;
        this->dataCPU.copyTo(cloneData);

        //		if(this->dataCPU.rows != 0){
        //		// this creates a copy of the entire data structure
        //		cloneData = this->dataCPU.clone();
        //		}

    } else {
        // this only creates a header and points to the same data as dataCPU
        // cloneData = this->dataCPU;
    }

    // set actual data into the data region
    clonedDataRegion->setData(cloneData);

    return clonedDataRegion;
}

long DenseDataRegion2D::getDataSize() {
    return this->dataCPU.rows * this->dataCPU.cols * this->dataCPU.elemSize();
}

int DenseDataRegion2D::getYDimensionSize() { return this->dataCPU.rows; }

bool DenseDataRegion2D::empty() {
    if (this->dataCPU.rows == 0 || this->dataCPU.cols == 0) {
        return true;
    }
    return false;
}

void DenseDataRegion2D::insertChukedData(BoundingBox bb, cv::Mat data) {
    this->chunkedDataCaching.insert(std::pair<BoundingBox, cv::Mat>(bb, data));
}
