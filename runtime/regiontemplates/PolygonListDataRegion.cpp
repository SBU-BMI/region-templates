//
// Created by taveira on 6/2/16.
//

#include "PolygonListDataRegion.h"

PolygonListDataRegion::PolygonListDataRegion() {
    this->setType(RegionTemplateType::POLYGON_LIST);
}

PolygonListDataRegion::~PolygonListDataRegion() {
#ifdef DEBUG
    std::cout << "POLYLISTDR destructor:" << this->getName() << ":" << this->getId() << std::endl;
#endif
    this->listOfPolygons.clear();
}

DataRegion *PolygonListDataRegion::clone(bool copyData) {
    PolygonListDataRegion *clonedDataRegion = new PolygonListDataRegion();
    // First, copy information stored into the DataRegion father's class.
    // This is done by serializing that class and deserializing it into
    // the "cloned" data region.

    // get size need to serialize fathers class
    int serializeSize = this->serializationSize();

    // create serialization buffer
    char *serBuffer = (char *) malloc(sizeof(char) * serializeSize);

    // serialize fathers class
    this->serialize(serBuffer);

    // deserialize information using the "clone object"
    clonedDataRegion->deserialize(serBuffer);
    // release buffer
    free(serBuffer);

    // create data clone.
    std::vector<std::vector<cv::Point> > cloneData(listOfPolygons);
    // copy data as well if copyData is set
    if (copyData) {

//		if(this->dataCPU.rows != 0){
//		// this creates a copy of the entire data structure
//		cloneData = this->dataCPU.clone();
//		}
        clonedDataRegion->setData(cloneData);

    } else {
        // this only creates a header and points to the same data as dataCPU
        //cloneData = this->dataCPU;
        std::vector<std::vector<cv::Point> > a;
        clonedDataRegion->setData(a);
    }

    // set actual data into the data region


    return clonedDataRegion;
}

void PolygonListDataRegion::setData(std::vector<std::vector<cv::Point> > vector) {
    this->listOfPolygons = vector;

    long size = 0;
    for (int i = 0; i < listOfPolygons.size(); ++i) {
        size += listOfPolygons.at(i).size();
    }
    this->cachedDataSize = ((this->listOfPolygons.size() + size) * sizeof(cv::Point)) +
                           ((listOfPolygons.size() + 1) * sizeof(std::vector<cv::Point>));

}

std::vector<std::vector<cv::Point> > PolygonListDataRegion::getData() {
    return this->listOfPolygons;
}

bool PolygonListDataRegion::empty() {
    return listOfPolygons.size() <= 0;

}

long PolygonListDataRegion::getDataSize() {
    long size = 0;
    for (int i = 0; i < listOfPolygons.size(); ++i) {
        size += listOfPolygons.at(i).size();
    }
    return ((this->listOfPolygons.size() + size) * sizeof(cv::Point)) +
           ((listOfPolygons.size() + 1) * sizeof(std::vector<cv::Point>));

}
