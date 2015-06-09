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

        TaskArgumentMat* maskTile = static_cast<TaskArgumentMat*>(this->getArgument(0));
        TaskArgumentMat* neighborArg = static_cast<TaskArgumentMat*>(this->getArgument(1));

        maskTile->setType(ExecEngineConstants::INPUT);


	if (procType == ExecEngineConstants::GPU) {// GPU
		Stream stream;
//		GpuMat g_mask(roiMask);
		std::cout << "maskTile->gpu_mat.cols:"<< maskTile->gpu_mat.cols << " maskTile.data==NULL?" << (maskTile->gpu_mat.data==NULL) << std::endl;
		std::cout << "maskTile->cpu_mat.cols:"<< maskTile->cpu_mat.cols << std::endl;
		//GpuMat g_distance = nscale::gpu::distanceTransform(maskTile->gpu_mat, stream, false, tileX, tileY, tileSize, neighbor->cols);

		neighborArg->gpu_mat = nscale::gpu::distanceTransform(maskTile->gpu_mat, stream, false, tileX, tileY, tileSize, neighbor->cols);
//		g_distance.download(roiNeighbor);
//		g_distance.release();
	//	g_mask.release();


	} else if (procType == ExecEngineConstants::CPU) { // CPU
		int microTile = 4096;
	//	Mat neighborMapTile = nscale::distanceTransformParallelTile(roiMask, microTile, 1, false);

		Mat neighborMapTile = nscale::distanceTransformParallelTile(maskTile->cpu_mat, microTile, 1, false);
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
//		neighborMapTile.copyTo(roiNeighbor);

		neighborMapTile.copyTo(neighborArg->cpu_mat);
	}

	uint64_t t1 = Util::ClockGetTimeProfile();
	std::cout << "TileRecon: procType:"<< procType<<" time="<<t1-t0<<std::endl;
		
}


