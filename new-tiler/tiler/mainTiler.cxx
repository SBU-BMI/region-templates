#include <cstdio>
#include <iostream>
#include <string>
#include <algorithm>

// openslide
#include "openslide.h"

// openCV
#include <opencv2/opencv.hpp>

// local
#include "utilitiesSvs.h"



int main(int argc, char **argv)
{
  if (argc < 4)
    {
      std::cerr<<"Parameters: imageName outputPrefix tileSize\n";
      exit(-1);
    }

  const int ImageDimension = 2;

  const char* fileName = argv[1];
  std::string outputPrefix(argv[2]);
  int64_t tileSize = strtoll(argv[3], NULL, 10);
  //std::cout<<tileSize<<std::endl;

  float threshold = 175.0;
  if (argc >= 5)
    {
      threshold = atof(argv[4]);
    }


  openslide_t *osr = openslide_open(fileName);

  int magnification = gth818n::getMagnification(osr);

  int64_t largestSizeW = 0;
  int64_t largestSizeH = 0;
  int32_t levelOfLargestSize = 0;

  gth818n::getLargestLevelSize(osr, levelOfLargestSize, largestSizeW, largestSizeH);

  std::cout<<"Largest level: "<< levelOfLargestSize<<" has size: "<<largestSizeW<<", "<<largestSizeH<<std::endl;

  int64_t nTileW = largestSizeW/tileSize + 1;
  int64_t nTileH = largestSizeH/tileSize + 1;

  for (int64_t iTileW = 0; iTileW < nTileW; ++iTileW)
    {
      for (int64_t iTileH = 0; iTileH < nTileH; ++iTileH)
        {
          std::cout<<iTileW<<", "<<iTileH<<std::endl<<std::flush;

          int64_t topLeftX = iTileW*tileSize;
          int64_t topLeftY = iTileH*tileSize;

          int64_t thisTileSizeX = std::min(tileSize, largestSizeW - topLeftX);
          int64_t thisTileSizeY = std::min(tileSize, largestSizeH - topLeftY);

          if (0 == thisTileSizeX || 0 == thisTileSizeY)
            {
              continue;
            }

          uint32_t* dest = new uint32_t[thisTileSizeX*thisTileSizeY];

          openslide_read_region(osr, dest, topLeftX, topLeftY, levelOfLargestSize, thisTileSizeX, thisTileSizeY);

          cv::Mat thisTile(thisTileSizeY, thisTileSizeX, CV_8UC3, cv::Scalar(0, 0, 0));
          gth818n::osrRegionToCVMat(dest, thisTile);

          delete[] dest;

          char tileName[1000];
          sprintf(tileName, "%s_%ld_%ld_%ld_%ld_%d.tif", outputPrefix.c_str(), tileSize, tileSize, topLeftX, topLeftY, magnification);
          cv::imwrite(tileName, thisTile);
        }
    }


  return 0;
}
