/*
 * TaskDoAll.cpp
 *
 *  Created on: Mar 3, 2012
 *      Author: george
 */

#include "TaskDoAll.h"

TaskDoAll::TaskDoAll(std::string inputFileName) {
	this->inputFileName = inputFileName;
}

TaskDoAll::~TaskDoAll() {
	
}

bool TaskDoAll::run(int procType, int tid) {

uint64_t t0 = Util::ClockGetTimeProfile();
	Mat inputImage = cv::imread(this->inputFileName);
	Mat outLabeledMask;
	int compcount;	
	Util::PrintElapsedTime("InputRead", t0);

	if (!inputImage.data) {
		std::cout << "ERROR: invalid image: "<< this->inputFileName << std::endl;
		return false;
	}else{
		std::cout << "Image was successfully load: "<< this->inputFileName << std::endl;
	}

	if (procType == ExecEngineConstants::GPU) {// GPU

		::cv::gpu::Stream stream;
		::cv::gpu::GpuMat g_labeledImage;
		::cv::gpu::GpuMat g_color;

		stream.enqueueUpload(inputImage, g_color);
		stream.waitForCompletion();

		int* g_bbox = NULL;

		std::cout << "gpu image.cols "<< g_color.cols << std::endl;
		int segmentationExecCode = nscale::gpu::HistologicalEntities::segmentNuclei(g_color, g_labeledImage, compcount, g_bbox);
		std::cout << "gpu after segmentation" << std::endl;

		if(segmentationExecCode == ::nscale::HistologicalEntities::CONTINUE || segmentationExecCode == ::nscale::HistologicalEntities::SUCCESS){
			vector<cv::gpu::GpuMat> g_bgr;
			cv::gpu::split(g_color, g_bgr);

			float* h_intensityFeatures = nscale::gpu::ObjFeatures::intensityFeatures(g_bbox,  compcount, g_labeledImage , g_bgr[0], stream);
			stream.waitForCompletion();
			float* h_intensityFeatures1 = nscale::gpu::ObjFeatures::intensityFeatures(g_bbox,  compcount, g_labeledImage , g_bgr[1], stream);
			stream.waitForCompletion();
			float* h_intensityFeatures2 = nscale::gpu::ObjFeatures::intensityFeatures(g_bbox,  compcount, g_labeledImage , g_bgr[2], stream);
			stream.waitForCompletion();
			free(h_intensityFeatures);
			free(h_intensityFeatures1);
			free(h_intensityFeatures2);

			float* g_h_gradientFeatures = nscale::gpu::ObjFeatures::gradientFeatures(g_bbox, compcount, g_labeledImage, g_bgr[0], stream);
			stream.waitForCompletion();
			float* g_h_gradientFeatures1 = nscale::gpu::ObjFeatures::gradientFeatures(g_bbox, compcount, g_labeledImage, g_bgr[1], stream);
			stream.waitForCompletion();
			float* g_h_gradientFeatures2 = nscale::gpu::ObjFeatures::gradientFeatures(g_bbox, compcount, g_labeledImage, g_bgr[2], stream);
			stream.waitForCompletion();
			free(g_h_gradientFeatures);
			free(g_h_gradientFeatures1);
			free(g_h_gradientFeatures2);

			float* g_h_cannyFeatures = nscale::gpu::ObjFeatures::cannyFeatures(g_bbox, compcount, g_labeledImage, g_bgr[0], stream);
			stream.waitForCompletion();
			float* g_h_cannyFeatures1 = nscale::gpu::ObjFeatures::cannyFeatures(g_bbox, compcount, g_labeledImage, g_bgr[1], stream);
			stream.waitForCompletion();
			float* g_h_cannyFeatures2 = nscale::gpu::ObjFeatures::cannyFeatures(g_bbox, compcount, g_labeledImage, g_bgr[2], stream);
			stream.waitForCompletion();
			free(g_h_cannyFeatures);
			free(g_h_cannyFeatures1);
			free(g_h_cannyFeatures2);

			g_bgr[0].release();
			g_bgr[1].release();
			g_bgr[2].release();
			std::cout <<"Features computed" <<std::endl;
			nscale::gpu::cudaFreeCaller(g_bbox);
		}

		g_color.release();
		g_labeledImage.release();

//		nscale::gpu::PixelOperations::testLeack();


	} else if (procType == ExecEngineConstants::CPU) { // CPU
		int *bbox = NULL;

		// Segmentation stage
		int segmentationExecCode = ::nscale::HistologicalEntities::segmentNuclei(inputImage, outLabeledMask, compcount, bbox);

		if(segmentationExecCode == ::nscale::HistologicalEntities::CONTINUE || segmentationExecCode == ::nscale::HistologicalEntities::SUCCESS){
			vector<cv::Mat> bgr;

			split(inputImage, bgr);

			// Do intensity features
			float* intensityFeatures = nscale::ObjFeatures::intensityFeatures(bbox, compcount, outLabeledMask, bgr[0]);
			float* intensityFeatures1 = nscale::ObjFeatures::intensityFeatures(bbox, compcount, outLabeledMask, bgr[1]);
			float* intensityFeatures2 = nscale::ObjFeatures::intensityFeatures(bbox, compcount, outLabeledMask, bgr[2]);
			free(intensityFeatures);
			free(intensityFeatures1);
			free(intensityFeatures2);

			// Do gradient features
			float* h_gradientFeatures = nscale::ObjFeatures::gradientFeatures(bbox, compcount, outLabeledMask, bgr[0]);
			float* h_gradientFeatures1 = nscale::ObjFeatures::gradientFeatures(bbox, compcount, outLabeledMask, bgr[1]);
			float* h_gradientFeatures2 = nscale::ObjFeatures::gradientFeatures(bbox, compcount, outLabeledMask, bgr[2]);
			free(h_gradientFeatures);
			free(h_gradientFeatures1);
			free(h_gradientFeatures2);

			// Do canny features
			float* h_cannyFeatures = nscale::ObjFeatures::cannyFeatures(bbox, compcount, outLabeledMask, bgr[0]);
			float* h_cannyFeatures1 = nscale::ObjFeatures::cannyFeatures(bbox, compcount, outLabeledMask, bgr[1]);
			float* h_cannyFeatures2 = nscale::ObjFeatures::cannyFeatures(bbox, compcount, outLabeledMask, bgr[2]);
			free(h_cannyFeatures);
			free(h_cannyFeatures1);
			free(h_cannyFeatures2);

			// just release data created by the split operation
			bgr[0].release();
			bgr[1].release();
			bgr[2].release();
			if(bbox!=NULL)
				free(bbox);

		}
		outLabeledMask.release();

	}


	inputImage.release();
		
}


