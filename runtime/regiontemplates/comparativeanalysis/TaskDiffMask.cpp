#include "TaskDiffMask.h"


TaskDiffMask::~TaskDiffMask() {
	if(dr1 != NULL) delete dr1;
	if(dr2 != NULL) delete dr2;
}


//void TaskDiffMask::setDr1(DenseDataRegion2D *dr1) {
//	this->dr1 = dr1;
//}
//
//void TaskDiffMask::setDr2(DenseDataRegion2D *dr2) {
//    this->dr2 = dr2;
//}
//
//void TaskDiffMask::setDiff(int *diff) {
//    this->diff = diff;
//}