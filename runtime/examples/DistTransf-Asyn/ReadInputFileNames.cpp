/*
 * ReadInputFileNames.cpp
 *
 *  Created on: Mar 2, 2012
 *      Author: george
 */

#include "ReadInputFileNames.h"

ReadInputFileNames::ReadInputFileNames() {
	// TODO Auto-generated constructor stub

}

ReadInputFileNames::~ReadInputFileNames() {
	// TODO Auto-generated destructor stub
}


void ReadInputFileNames::getFiles(const std::string &imageName, const std::string &outDir, std::vector<std::string> &filenames,
		std::vector<std::string> &seg_output) {

	// check to see if it's a directory or a file
	std::string suffix;
	suffix.assign(".tif");

	FileUtils futils(suffix);
	futils.traverseDirectoryRecursive(imageName, filenames);
	std::string dirname = imageName;
	if (filenames.size() == 1) {
		// if the maskname is actually a file, then the dirname is extracted from the maskname.
		if (strcmp(filenames[0].c_str(), imageName.c_str()) == 0) {
			dirname = imageName.substr(0, imageName.find_last_of("/\\"));
		}
	}

	std::string temp, tempdir;
	for (unsigned int i = 0; i < filenames.size(); ++i) {
		// generate the output file name
		temp = futils.replaceExt(filenames[i], ".tif", ".mask.pbm");
		temp = futils.replaceDir(temp, dirname, outDir);
		tempdir = temp.substr(0, temp.find_last_of("/\\"));
		futils.mkdirs(tempdir);
		seg_output.push_back(temp);
	}

}

void ReadInputFileNames::getDimensions(int& x, int& y, std::vector<std::string>& fileNames) {
	x = 0;
	y = 0;
	for(int i = 0; i < fileNames.size(); i++){
//		char myString[] = "The quick brown fox";
		size_t found;
//		std::cout << "Splitting: " << str << std::endl;
		found=fileNames[i].find_last_of("/\\");
//		  cout << " folder: " << str.substr(0,found) << endl;
		std::string fileName = fileNames[i].substr(found+1);
		std::cout << "fileName="<<fileName<<std::endl;


		char *p = strtok((char*)fileName.c_str(), "-");

		int it = 1;
		while (p) {

		    p = strtok(NULL, "-");
		    if(p==NULL)break;
		    it++;
//		     std::cout <<"Token: " << p << std::endl;
		    if(it == 7){
//		        std::cout <<"Token: " << p << std::endl;
		        if (atoi(p) > x){
		        	x = atoi(p);
		        }
		    }
		    if(it == 8){
		    	p  = strtok(p, ".");
//		        std::cout <<"Token: " << p << std::endl;
		        if (atoi(p) > y){
		        	y = atoi(p);
		        }
		    }
		}

	}
}

void ReadInputFileNames::assembleTiles(cv::Mat& image,
		std::vector<std::string>& fileNames) {
	for(int i = 0; i < fileNames.size(); i++){
		size_t found;
		found=fileNames[i].find_last_of("/\\");

		std::string fileName = fileNames[i].substr(found+1);


		char *p = strtok((char*)fileName.c_str(), "-");
		int x=0, y=0;
		int it = 1;
		while (p) {
		    p = strtok(NULL, "-");
		    if(p==NULL)break;
		    it++;
		    if(it == 7)x = atoi(p);
		    if(it == 8){
		    	p  = strtok(p, ".");
	        	y = atoi(p);
		    }
		}
		std::cout << "fileName="<<fileNames[i]<<" x="<<x<<" y="<<y<<std::endl;

		cv::Mat temp = cv::imread(fileNames[i], -1);
		cv::Mat roiMask(image, cv::Rect(x, y , temp.cols, temp.rows));
		if(temp.data == NULL){
			std::cout << "failedReadingFiles"<<std::endl;
		}
			temp.copyTo(roiMask);
		if(i==1){
			imwrite("roi.pgm", roiMask);

			imwrite("roi.tiff", roiMask);

		}



	}
}


