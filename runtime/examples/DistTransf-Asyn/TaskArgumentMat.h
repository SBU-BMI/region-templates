/*
 * TaskArgumentMat.h
 *
 *  Created on: Apr 3, 2012
 *      Author: gteodor
 */

#ifndef TASKARGUMENTMAT_H_
#define TASKARGUMENTMAT_H_
#include "TaskArgument.h"

#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"


class TaskArgumentMat: public TaskArgument {
public:
	cv::Mat cpu_mat;
	cv::gpu::GpuMat gpu_mat;

	TaskArgumentMat();
	virtual ~TaskArgumentMat();

	bool upload(cv::gpu::Stream& stream);
	bool download(cv::gpu::Stream& stream);
	void setCPUMat(cv::Mat cpu_mat);
	void deleteGPUData();
};

#endif /* TASKARGUMENTMAT_H_ */
