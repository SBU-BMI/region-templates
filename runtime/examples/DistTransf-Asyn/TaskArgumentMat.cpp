/*
 * TaskArgumentMat.cpp
 *
 *  Created on: Apr 3, 2012
 *      Author: gteodor
 */

#include "TaskArgumentMat.h"

TaskArgumentMat::TaskArgumentMat() {
	cpu_mat.data = NULL;
	gpu_mat.data = NULL;
}

TaskArgumentMat::~TaskArgumentMat() {
	if(gpu_mat.data != NULL){
		std::cout << "ERROR: data not released. Rows = "<< gpu_mat.rows << " argId="<< this->getId() << std::endl;
		//		gpu_mat.release();
	}
//	if(cpu_mat != NULL){
//		if(cpu_mat.data != NULL){
//			cpu_mat.release();
//		}
//		delete cpu_mat;
//		cpu_mat = NULL;
//	}
//	delete cpu_mat;

}

bool TaskArgumentMat::upload(cv::gpu::Stream& stream) {
	std::cout<< "Upload called. Arg id = "<< this->getId()<<std::endl;
	if(gpu_mat.data == NULL && cpu_mat.data != NULL && getType() != ExecEngineConstants::OUTPUT){
		gpu_mat = cv::gpu::createContinuous(cpu_mat.size(), cpu_mat.type());
		stream.enqueueUpload(cpu_mat, gpu_mat);
	}else{
//		std::cout << "Upload. Nothing todo"<<std::endl;
	}
}

bool TaskArgumentMat::download(cv::gpu::Stream& stream) {
	std::cout<< "Download called. Arg id = "<< this->getId()<<std::endl;

	if(gpu_mat.data != NULL){
//		cpu_mat.create(gpu_mat.size(), gpu_mat.type());
		stream.enqueueDownload(gpu_mat, cpu_mat);
	}else{
//		std::cout << "download. Nothing todo"<<std::endl;
	}
}

void TaskArgumentMat::setCPUMat(cv::Mat cpu_mat) {
	this->cpu_mat = cpu_mat;
}

void TaskArgumentMat::deleteGPUData() {
//	std::cout << "Delete son GPUData. argId="<< this->getId() << std::endl;
	if(gpu_mat.data != NULL){
		gpu_mat.release();
	}
	gpu_mat.data = NULL;
}




