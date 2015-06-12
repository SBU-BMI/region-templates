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
	if(argc < 2){
		std::cout << "Usage: ./convert <image_file | image_dir >" << std::endl;
		return 1;
	}
        // Initialize input dir/image and outDir from user parameters
        std::string img_dir_path = argv[1], outDir = "/luster/medusa/gteodor/out/";

	// read files with "tif" extension in the given path
        ReadInputFileNames::getFiles(img_dir_path, outDir, input_file_names, seg_output);

	std::cout << "seg_output: "<< seg_output[0] << " dir_path:"<< img_dir_path<< std::endl;

	// Startup MPI
	MPI_Init (&argc, &argv);
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);	/* get current process id */
	MPI_Comm_size (MPI_COMM_WORLD, &size);	/* get number of processes */

	std::cout << "rank:"<< rank << " size:"<< size << std::endl;

	for(int i = rank; i < input_file_names.size(); i+=size){
		cv::Mat inputImage = cv::imread(input_file_names[i]);
	        if (!inputImage.data) {
        	        std::cout << "ERROR: invalid image: "<< input_file_names[i] << std::endl;
                	return false;
	        }
		std::string outName = input_file_names[i];
		outName.append(".pbm");
		cv::imwrite(outName, inputImage);
	}
	MPI_Finalize();
	printf(" Finalize Rank:%d\n", rank);
	return 0;
}
