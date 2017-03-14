////////////////////////////////////////////////////////////////////////////////
/// @file utilitiesSvs.cxx
/// @author  Yi Gao <gaoyi@gatech.edu>
/// @version 1.0
///
/// @section LICENSE
///
/// This program is free software; you can redistribute it and/or
/// modify it under the terms of the GNU General Public License as
/// published by the Free Software Foundation; either version 2 of
/// the License, or (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful, but
/// WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
/// General Public License for more details at
/// http://www.gnu.org/copyleft/gpl.html
///
/// @section DESCRIPTION
/// The functions in this file are grouped by the creteria that any
/// function that has ANYTHING to do with SVS will be put here.
////////////////////////////////////////////////////////////////////////////////


// openslide
#include "openslide.h"

// openCV
#include <opencv2/opencv.hpp>

// local
#include "utilitiesSvs.h"


namespace gth818n
{
  int getMagnification(openslide_t *osr)
  {
    const char magnificationStringName[] = "aperio.AppMag";
    int magnification = 0;
    const char * const *property_names = openslide_get_property_names(osr);
    while (*property_names)
      {
        const char *name = *property_names;
        const char *value = openslide_get_property_value(osr, name);

        if (!strcmp(magnificationStringName, name))
          {
            magnification = atof(value);
            //printf("%s: '%d'\n", name, magnification);

            break;
          }

        property_names++;
      }

    return magnification;
  }

  void getLargestLevelSize(openslide_t *osr, int32_t& levelOfLargestSize, int64_t& largestSizeW, int64_t& largestSizeH)
  {
    int32_t numberOfLevels = openslide_get_level_count(osr);
    //std::cout<<"Totally "<<numberOfLevels<<" levels.\n";

    {
      int64_t w[1];
      int64_t h[1];

      openslide_get_level_dimensions(osr, 0, w, h);

      largestSizeW = w[0];
      largestSizeH = h[0];

      for (int32_t iLevel = 1; iLevel < numberOfLevels; ++iLevel)
        {
          int64_t w1[1];
          int64_t h1[1];

          openslide_get_level_dimensions(osr, iLevel, w1, h1);

          if (w1[0]*h1[0] > largestSizeW*largestSizeH)
            {
              levelOfLargestSize = iLevel;
              largestSizeW = w1[0];
              largestSizeH = h1[0];
            }
        }
    }

    return;
  }

  void osrRegionToCVMat(const uint32_t* osrRegion, cv::Mat thisTile)
  {
    int64_t numOfPixelPerTile = thisTile.total();

    for (int64_t it = 0; it < numOfPixelPerTile; ++it)
      {
        uint32_t p = osrRegion[it];

        uint8_t a = (p >> 24) & 0xFF;
        uint8_t r = (p >> 16) & 0xFF;
        uint8_t g = (p >> 8) & 0xFF;
        uint8_t b = p & 0xFF;

        switch (a) {
        case 0:
          r = 0;
          b = 0;
          g = 0;
          break;

        case 255:
          // no action needed
          break;

        default:
          r = (r * 255 + a / 2) / a;
          g = (g * 255 + a / 2) / a;
          b = (b * 255 + a / 2) / a;
          break;
        }

        // write back
        thisTile.at<cv::Vec3b>(it)[0] = b;
        thisTile.at<cv::Vec3b>(it)[1] = g;
        thisTile.at<cv::Vec3b>(it)[2] = r;
      }

    return;
  }
}// namepspace
