/*
 * Constants.h
 *
 *  Created on: Oct 22, 2012
 *      Author: george
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

// OpenCV library includes
#include "cv.hpp"
#include "opencv2/gpu/gpu.hpp"



class DataElementType {
private:

	DataElementType();
	virtual ~DataElementType();
public:
	static const int UCHAR 	= CV_8U; // unsigned char
	static const int SCHAR	= CV_8UC1; // signed char
	static const int USHORT	= CV_16U; // unsigned short
	static const int SSHORT	= CV_16S; //signed short
	static const int INT	= CV_32S; //int
	static const int FLOAT	= CV_32F; //float
	static const int DOUBLE	= CV_64F; //double
};

class Environment {
private:
	Environment();
	virtual ~Environment();

public:
	static const int CPU 	= 0; // CPU device
	static const int GPU 	= 1; // GPU device

};

/*class RegionTemplateType {
private:
	RegionTemplateType();
	virtual ~RegionTemplateType();

public:
	static const int DENSE_REGION_2D 	= 1; // Dense matrix representation
	static const int REGION_2D_UNALIGNED = 2;
	static const int DENSE_REGION_3D 	= 2; // Dense (cube) array of matrices

};*/

class DataRegionType{
private:
	DataRegionType();
	virtual ~DataRegionType();

public:
	static const int DENSE_REGION_2D 	= 1; // Dense matrix representation
	static const int REGION_2D_UNALIGNED = 2;
	static const int DENSE_REGION_3D 	= 3; // Dense (cube) array of matrices
};

class DataSourceType{
private:
	DataSourceType();
	virtual ~DataSourceType();

public:
	static const int FILE_SYSTEM	= 1; // when data is coming from the file systems
	static const int DATA_SPACES	= 2; // if data is coming from data spaces
	static const int FILE_SYSTEM_TEXT_FILE	= 3; // if the image is coming from the file system inside a text file
};

#endif /* CONSTANTS_H_ */
