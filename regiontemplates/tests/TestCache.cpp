
#include <string>
#include <iostream>
#include <omp.h>

#include "RegionTemplate.h"
//#include "ImageFileChunksDataSource.h"
#include "gtest/gtest.h"
#include "DenseDataRegion2D.h"
#include "Constants.h"
#include "CacheComponent.h"
#include "Cache.h"
//#include "ImageFileChunksDataSource.h"

int main(int argc, char **argv){

	if(argc != 2){
		std::cout << "Usage: ./TestCache <inputFileName>" << std::endl;
	}
	DenseDataRegion2D *dr2D = new DenseDataRegion2D();
	dr2D->setName("drTest");
	dr2D->setId("image-test");
	dr2D->setTimestamp(1);
	dr2D->setVersion(2);
	cv::Mat inputData = cv::imread(argv[1], -1);
	if(inputData.data == NULL){
		std::cout << "Failed to load image!!" << std::endl;
		exit(1);
	}
	cv::Mat inputclone = inputData.clone();

	inputData.release();
	dr2D->setData(inputclone);

	Cache *cache  = new Cache("rtconf.xml");
#pragma omp parallel for
	for(int i = 0; i < 10; i++){

		cache->insertDR("rt", "1", dr2D->getName(), dr2D, true);
		dr2D->setVersion(dr2D->getVersion()+1);
	}

//	for(int j = 0; j < 1; j++){
#pragma omp parallel for
		for(int i = 0; i < 10; i++){
			int nThreads = omp_get_num_threads();
			std::cout << "NUM_THREAD: " << nThreads << std::endl;
			DenseDataRegion2D *aux = dynamic_cast<DenseDataRegion2D*>(cache->getDR("rt", "1", dr2D->getName(),dr2D->getTimestamp(), 2+i, false));
			if(aux != NULL){
				std::cout << "DATA region found: "<< std::endl;
				aux->print();
				delete aux;

				bool moveRet = cache->move2Global("rt", "1", dr2D->getName(), dr2D->getTimestamp(), 1+i);
				if(moveRet!= false){
					std::cout << "move2global: rt, 1 "<< dr2D->getName() << " "<< dr2D->getTimestamp()<<" "<<2+i <<std::endl;
				}
			}else{
				std::cout << "DATA region not found: version: "<< 2+i<<std::endl;

			}
		}
	//}
	delete cache;
	delete dr2D;


	return 0;

}
