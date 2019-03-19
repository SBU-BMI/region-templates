#ifndef DISTILING_H
#define DISTILING_H 

#include "mpi.h"
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include "Halide.h"

#include "PriorityQ.h"

#define MPI_TAG 0
#define MPI_MANAGER_RANK 0

typedef struct rect_t {
	int xi, yi;
	int xo, yo;
} rect_t;

typedef struct thr_args_t {
	int currentRank;
    cv::Mat *input;
    std::list<rect_t> *rQueue;
} thr_args_t;

int distExec(int argc, char* argv[], cv::Mat& inImg);

#include "autoTiler.h" // here to solve circular dependency problem

#endif