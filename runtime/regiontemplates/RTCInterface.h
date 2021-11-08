#ifndef RTCINTERFACE_H_
#define RTCINTERFACE_H_

#include "Worker.h"
// #include <cv.h>

void printComponentHello();

// compId: id of the component instance. Used to retrieve component instance object being executed.
cv::Mat* getDataRegion(string compId, string rtName, string drName, string drId, int timeStamp, int x=-1, int y=-1, int width=-1, int height=-1);

//inline void printComponentHello();

#endif
