
#include <string>
#include <iostream>
#include "opencv2/opencv.hpp"
#include <iostream>
#include <dirent.h>
#include <vector>
#include <errno.h>
#include <time.h>

int main(int argc, char **argv){
	std::string type = "LOCAL";
	cv::Mat testWrite;
	cv::FileStorage fs("mat.xml", cv::FileStorage::WRITE);

	fs << "CacheConfig"<< "[";
	for(int i =0 ; i < 3; i++){
		fs << "{:"<<"type"<< type;
		fs << "device" << "disk";
		fs << "size"   << 1500;
		fs << "path" << "/usr/data";
		fs << "level"<< 1;
		//fs << "mat" << testWrite;
		fs<< "}";
	}
	fs<<"]";

	fs.release();


	cv::FileStorage fs2("mat2.xml", cv::FileStorage::READ);
	if(fs2.isOpened()){
		cv::FileNode cacheConf = fs2["CacheConfig"];
		cv::FileNodeIterator it = cacheConf.begin(), itEnd = cacheConf.end();
		int idx = 0;
		for(; it != itEnd; it++){
			std::cout << "type="<< (std::string)(*it)["type"]<< " size="<< (int)(*it)["size"]
			                                                                          << " path=" << (std::string)(*it)["path"] << " device="<< (std::string)(*it)["device"]
			                                                                                                                                                       << " level=" << (int)(*it)["level"]<< std::endl;
		}
	}else{
		std::cout << "Could not find input config file" << std::endl;
	}

	return 0;

}
