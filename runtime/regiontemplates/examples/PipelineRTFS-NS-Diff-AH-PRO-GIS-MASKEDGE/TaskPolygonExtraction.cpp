//
// Created by taveira on 6/5/16.
//

#include <regiontemplates/comparativeanalysis/hadoopgis/Hadoopgis.h>
#include <regiontemplates/DataRegionFactory.h>
#include "TaskPolygonExtraction.h"

TaskPolygonExtraction::TaskPolygonExtraction(DenseDataRegion2D *refmask, PolygonListDataRegion *polygonListDataRegion) {
    this->refmask = refmask;
    this->polygonListDataRegion = polygonListDataRegion;
}

TaskPolygonExtraction::~TaskPolygonExtraction() {
    //
    if (refmask != NULL) delete refmask;
}

bool TaskPolygonExtraction::run(int procType, int tid) {
    cv::Mat inputImage = this->refmask->getData();
    // target values computed from the reference image
    std::vector<std::vector<cv::Point> > *listOfPoly;
    uint64_t t1 = Util::ClockGetTimeProfile();
    Hadoopgis::getPolygonsFromMask(inputImage, listOfPoly);
    //std::cout << "*********************** Size 0 of list " << listOfPoly->size()<< std::endl;

    uint64_t t2 = Util::ClockGetTimeProfile();
    this->polygonListDataRegion->setData(std::vector<std::vector<cv::Point> >(*listOfPoly));




//    std::cout << "################################ BEFORE WRITING TO FILE ################################ ";
//    std::cout << "Size of list before writing" << this->polygonListDataRegion->getData().size()<< std::endl;
//    DataRegionFactory::writePolyListDRFS(polygonListDataRegion, "/tmp/");
//
//    std::cout << "################################ AFTER READING FROM FILE ################################ ";
//    PolygonListDataRegion* temp = (PolygonListDataRegion*) polygonListDataRegion->clone(false);
//    DataRegionFactory::readPolyListDRFS(temp, "/tmp/");
//    std::cout << "Size of list after reading " << temp->getData().size()<< std::endl;
//


    //std::cout << "*********************** Size 1 of list " << this->polygonListDataRegion->getData().size()<< std::endl;

    std::cout << "Task Polygon Extraction time elapsed: " << t2 - t1 << std::endl;

}
