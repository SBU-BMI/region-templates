////////////////////////////////////////////////////////////////////////////////
/// @file utilitiesSvs.h
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



#ifndef utilitiesSvs_h_
#define utilitiesSvs_h_


// openslide
#include "openslide.h"

// openCV
#include <opencv2/opencv.hpp>


namespace gth818n
{
  int getMagnification(openslide_t *osr);

  void getLargestLevelSize(openslide_t *osr, int32_t& levelOfLargestSize, int64_t& largestSizeW, int64_t& largestSizeH);

  void osrRegionToCVMat(const uint32_t* osrRegion, cv::Mat thisTile);

}// namepspace


#endif
