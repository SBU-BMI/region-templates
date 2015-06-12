/* C++ Example */
#include <stdio.h>
#include <vector>
#include <mpi.h>
#include "util.h" 
#include "assert.h" 
#include "ReadInputFileNames.h"
#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "Util.h"

extern "C" {
#include "dataspaces.h"
}


#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

/* The arguments: npapp - number of processors in the application */

int main (int argc, char **argv){
	int rank, size;
	std::vector<std::string> input_file_names, seg_output;

	// dataspaces parameters
	int npapp;
	if(argc < 3){
		std::cout << "Usage: ./test_put_images_2D <#npapp> <image_file | image_dir >" << std::endl;
		return 1;
	}
	npapp = atoi(argv[1]);
        // Initialize input dir/image and outDir from user parameters
        std::string img_dir_path = argv[2], outDir = "";

	// read files with "tiff" extension in the given path
        ReadInputFileNames::getFiles(img_dir_path, outDir, input_file_names, seg_output);

	// Startup MPI
	MPI_Init (&argc, &argv);
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);	/* get current process id */
	MPI_Comm_size (MPI_COMM_WORLD, &size);	/* get number of processes */

	long long begin = Util::ClockGetTime();
	dspaces_init(npapp, 2);
//	rank = dspaces_rank();
//	int peers = dspaces_peers();
	long long end = Util::ClockGetTime();
	std::cout << "DSpace Init Time: "<< end-begin << std::endl;
	std::cout << "rank:"<< rank << " peers:"<< size << std::endl;

	dspaces_barrier();

	long long totalReadTime = 0;

	dspaces_barrier();
	std::cout <<"AFTER BARRIER" << std::endl;
	sleep(10);
dspaces_lock_on_read("common_lock");
	long long totalReadTimeNeighborNode = 0;
	for(int i = rank; i < input_file_names.size(); i+=size){
		char *matrix = (char*)malloc(sizeof(char) * 4096 * 3 * 4096);

		std::cout << "Read img: "<< input_file_names[i] << std::endl;
		begin = Util::ClockGetTime();
		dspaces_get(input_file_names[i].c_str(), 1, sizeof(char), 0, 0, 0, (4096*3)-1, 4095, 0, matrix);
		end = Util::ClockGetTime();

		// testing if data returned from space is correct 	
//        	cv::Mat inputImage = cv::imread(input_file_names[i]);
//		char * imageData = inputImage.ptr<char>(0);
//		for(int x = 0; x < 4096  * 4096; x++){
//			if(imageData[x] != matrix[x]){
//			 	std::cout << "failed comparison: image:"<< (int)imageData[x] <<" matrix:"<<(int)matrix[x] << " x:"<<x << std::endl;
//				exit(1);
//			}
//
//		}
		totalReadTimeNeighborNode+=(end-begin);
//		inputImage.release();
		free(matrix);
	}

dspaces_unlock_on_read("common_lock");
	long long totalReadTimeSameNode = 0;
//	for(int i = rank; i < input_file_names.size(); i+=peers){
//		char *matrix = (char*)malloc(sizeof(char) * 4096 * 3 * 4096);
//
//		begin = Util::ClockGetTime();
//		dspaces_get(input_file_names[i].c_str(), 1, sizeof(char), 0, 0, 0, 12287, 4095, 0, matrix);
//		end = Util::ClockGetTime();
//	
//		totalReadTimeSameNode+=(end-begin);
//		free(matrix);
//	}
	

//	std::cout << "Rank:"<< rank << " imgReadTime:" << totalReadTime << " submitTime:"<< totalSubmitTime << " dataSpaceReadSameNode: "<< totalReadTimeSameNode << " dataSpaceReadNeighborNode: "<< totalReadTimeNeighborNode<< std::endl;

	begin = Util::ClockGetTime();
	dspaces_barrier();

	printf("After barrier Rank:%d\n", rank);
	dspaces_finalize();

	printf("After finalize Rank:%d\n", rank);
	MPI_Finalize();
	end = Util::ClockGetTime();
	std::cout << "DSpace Finalize Time: "<< end-begin << std::endl;
	
	printf(" Finalize Rank:%d\n", rank);
	return 0;
}
