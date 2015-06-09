/*
 * TaskDoAll.cpp
 *
 *  Created on: Mar 3, 2012
 *      Author: george
 */

#include "TaskTileProp.h"

TaskTileProp::TaskTileProp(::cv::Mat* neighbor, ::cv::Mat* maskImage, int tileX, int tileY, int tileSize, int imgCols) {
	this->neighbor = neighbor;
	this->maskImage = maskImage;

	this->tileX = tileX;
	this->tileY = tileY;
	this->tileSize = tileSize;
	this->imgCols = imgCols;
}

TaskTileProp::~TaskTileProp() {

}

bool TaskTileProp::run(int procType, int tid) {

	std::cout << "TileX:"<<tileX<<" TileY:"<<tileY<<" TileSize:"<<tileSize<<std::endl;
	uint64_t t0 = Util::ClockGetTimeProfile();

	Mat roiMask((*maskImage), Rect(tileX*tileSize, tileY*tileSize , tileSize, tileSize));
	Mat roiNeighbor((*neighbor), Rect(tileX*tileSize, tileY*tileSize , tileSize, tileSize));

	std::cout << "roiMask.rows:"<< roiMask.rows << " roiMask.cols:" << roiMask.cols<< " Channels:" << roiMask.channels() <<std::endl;

	if (procType == ExecEngineConstants::GPU) {// GPU
		Stream stream;
		GpuMat g_mask(roiMask);

		GpuMat g_distance = nscale::gpu::distanceTransform(g_mask, stream, false, tileX, tileY, tileSize, neighbor->cols);

		g_distance.download(roiNeighbor);
		g_distance.release();
		g_mask.release();


	} else if (procType == ExecEngineConstants::CPU) { // CPU
		int microTile = 8192;
		Mat neighborMapTile = nscale::distanceTransformParallelTile(roiMask, microTile, 1, false);

		//Mat neighborMapTile = nscale::distanceTransform(roiMask, false);

		// code to calculate global address of pixels, from tile addr to image addr
		for(int y = 0; y < neighborMapTile.rows; y++){
			int* NMTPtr = neighborMapTile.ptr<int>(y);
 			for(int x = 0; x < neighborMapTile.cols; x++){
                        	int colId = NMTPtr[x] % neighborMapTile.cols + tileX * tileSize;
                                int rowId = NMTPtr[x] / neighborMapTile.cols + tileY * tileSize;
                                NMTPtr[x] = rowId*this->imgCols + colId;
			} 
		}
		neighborMapTile.copyTo(roiNeighbor);

	}

	uint64_t t1 = Util::ClockGetTimeProfile();
	std::cout << "TileRecon: procType:"<< procType<<" time="<<t1-t0<<std::endl;
		
}


