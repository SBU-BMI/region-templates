/*
 * TaskDoAll.cpp
 *
 *  Created on: Mar 3, 2012
 *      Author: george
 */

#include "TaskTileRecon.h"

TaskTileRecon::TaskTileRecon(::cv::Mat* markerImage, ::cv::Mat* maskImage, int tileX, int tileY, int tileSize) {
	this->markerImage = markerImage;
	this->maskImage = maskImage;

	this->tileX = tileX;
	this->tileY = tileY;
	this->tileSize = tileSize;
}

TaskTileRecon::~TaskTileRecon() {

}

bool TaskTileRecon::run(int procType, int tid) {

	std::cout << "TileX:"<<tileX<<" TileY:"<<tileY<<" TileSize:"<<tileSize<<std::endl;
	uint64_t t0 = Util::ClockGetTimeProfile();

	Mat roiMarker((*markerImage), Rect(tileX*tileSize+1, tileY*tileSize+1, tileSize, tileSize ));
	Mat roiMask((*maskImage), Rect(tileX*tileSize+1, tileY*tileSize+1, tileSize, tileSize));

	std::cout << "roiMarker.rows:"<<roiMarker.rows<< " roiMarker.cols:" <<roiMarker.cols<< " Channels:"<<roiMarker.channels()<<std::endl;

	if (procType == ExecEngineConstants::GPU) {// GPU
		uint64_t t_init = Util::ClockGetTimeProfile();
		Stream stream;
		GpuMat g_marker, g_mask, g_recon;
		stream.enqueueUpload(roiMarker, g_marker);
		stream.enqueueUpload(roiMask, g_mask);
		stream.waitForCompletion();
		uint64_t t_end = Util::ClockGetTimeProfile();
		std::cout << "Upload: "<< t_end-t_init << std::endl;

		g_recon = nscale::gpu::imreconstructQueueSpeedup<unsigned char>(g_marker, g_mask, 8, 6,stream,16);
//		g_recon = nscale::gpu::imreconstructQueueSpeedup<unsigned char>(g_marker, g_mask, 4, 20,stream,16);
		std::cout << "comp: "<< Util::ClockGetTimeProfile()-t_end <<std::endl;
		stream.waitForCompletion();
		t_init = Util::ClockGetTimeProfile();
		Mat reconTile;
		g_recon.download(reconTile);
		stream.waitForCompletion();
		t_end = Util::ClockGetTimeProfile();
		std::cout << "Download: "<< t_end-t_init << std::endl;
		reconTile.copyTo(roiMarker);


	} else if (procType == ExecEngineConstants::CPU) { // CPU

		Mat reconTile = nscale::imreconstructParallelTile<unsigned char>(roiMarker, roiMask, 8,2048,12);
//		Mat reconTile = nscale::imreconstruct<unsigned char>(roiMarker, roiMask, 8);
		//Mat reconTile = nscale::imreconstruct<unsigned char>(roiMarker, roiMask, 4);

//		Mat reconTile = nscale::imreconstructParallelTile<unsigned char>(roiMarker, roiMask, 4,2048,8);
		reconTile.copyTo(roiMarker);
	}

	uint64_t t1 = Util::ClockGetTimeProfile();
	std::cout << "TileRecon: procType:"<< procType<<" time="<<t1-t0<<std::endl;
		
}


