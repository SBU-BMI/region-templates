#include "TaskSegmentation.h"

#include "ConnComponents.h"

TaskSegmentation::TaskSegmentation(DenseDataRegion2D* bgr, 
	DenseDataRegion2D* mask, unsigned char blue, unsigned char green, 
	unsigned char red, double T1, double T2, unsigned char G1, unsigned char G2, 
	int minSize, int maxSize, int minSizePl, int minSizeSeg, int maxSizeSeg, 
	int fillHolesConnectivity, int reconConnectivity, int watershedConnectivity) {

	this->bgr = bgr;
	this->mask = mask;

	this->blue = blue;
	this->green = green;
	this->red = red;

	this->T1 = T1;
	this->T2 = T2;
	this->G1 = G1;
	this->G2 = G2;
	this->minSize = minSize;
	this->maxSize = maxSize;
	this->minSizePl = minSizePl;
	this->minSizeSeg = minSizeSeg;
	this->maxSizeSeg = maxSizeSeg;
	this->fillHolesConnectivity=fillHolesConnectivity;
	this->reconConnectivity=reconConnectivity;
	this->watershedConnectivity=watershedConnectivity;
}

TaskSegmentation::~TaskSegmentation() {
	if(bgr != NULL) delete bgr;
}

int segmentNuclei(const Mat& img, Mat& output, unsigned char blue, 
	unsigned char green, unsigned char red, double T1, double T2, 
	unsigned char G1, int minSize, int maxSize, unsigned char G2, int minSizePl, 
	int minSizeSeg, int maxSizeSeg,  int fillHolesConnectivity, 
	int reconConnectivity, int watershedConnectivity, 
	::cciutils::SimpleCSVLogger *logger=NULL, 
	::cciutils::cv::IntermediateResultHandler *iresHandler=NULL);

bool TaskSegmentation::run(int procType, int tid) {
	cv::Mat inputImage = this->bgr->getData();
	cv::Mat outMask;

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "TaskSegmentation: " << (int)blue << ":"<< (int)green 
		<< ":"<< (int)red << ":"<< T1<< ":"<< T2<< ":"<< (int)G1<< ":"
		<< minSize<< ":"<< maxSize<< ":"<< (int)G2<< ":"<< minSizePl
		<< ":"<< minSizeSeg<< ":"<< maxSizeSeg << ":" << fillHolesConnectivity 
		<< ":" << reconConnectivity << ":" << watershedConnectivity << std::endl;
	if(inputImage.rows > 0)
		int segmentationExecCode = segmentNuclei(inputImage, outMask, blue, 
			green, red, T1, T2, G1, minSize, maxSize, G2, minSizePl, minSizeSeg, 
			maxSizeSeg, fillHolesConnectivity, reconConnectivity, 
			watershedConnectivity);
	else
		std::cout <<"Segmentation: input data NULL"<< std::endl;
	this->mask->setData(outMask);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}

/*****************************************************************************/
/*** Nscale workaround:                                                    ***/
/*** Nscale only works for opencv 2.4.9. This way we can compile only what ***/
/*** we need in opencv 3.4.1.                                              ***/
/*****************************************************************************/

template <typename T>
inline void propagateBinary(const Mat& image, Mat& output, std::queue<int>& xQ, std::queue<int>& yQ,
		int x, int y, T* iPtr, T* oPtr, const T& foreground) {
	if ((oPtr[x] == 0) && (iPtr[x] != 0)) {
		oPtr[x] = foreground;
		xQ.push(x);
		yQ.push(y);
	}
}

template
void propagateBinary(const Mat&, Mat&, std::queue<int>&, std::queue<int>&,
		int, int, unsigned char* iPtr, unsigned char* oPtr, const unsigned char&);
template
void propagateBinary(const Mat&, Mat&, std::queue<int>&, std::queue<int>&,
		int, int, float* iPtr, float* oPtr, const float&);

template <typename T>
Mat imreconstructBinary(const Mat& seeds, const Mat& image, int connectivity) {
	CV_Assert(image.channels() == 1);
	CV_Assert(seeds.channels() == 1);

	Mat output(seeds.size() + Size(2,2), seeds.type());
	copyMakeBorder(seeds, output, 1, 1, 1, 1, BORDER_CONSTANT, 0);
	Mat input(image.size() + Size(2,2), image.type());
	copyMakeBorder(image, input, 1, 1, 1, 1, BORDER_CONSTANT, 0);

	T pval, ival;
	int xminus, xplus, yminus, yplus;
	int maxx = output.cols - 1;
	int maxy = output.rows - 1;
	std::queue<int> xQ;
	std::queue<int> yQ;
	T* oPtr;
	T* oPtrPlus;
	T* oPtrMinus;
	T* iPtr;
	T* iPtrPlus;
	T* iPtrMinus;

//	uint64_t t1 = cci::common::event::timestampInUS();

	int count = 0;
	// contour pixel determination.  if any neighbor of a 1 pixel is 0, and the image is 1, then boundary
	for (int y = 1; y < maxy; ++y) {
		oPtr = output.ptr<T>(y);
		oPtrPlus = output.ptr<T>(y+1);
		oPtrMinus = output.ptr<T>(y-1);
		iPtr = input.ptr<T>(y);

		for (int x = 1; x < maxx; ++x) {

			pval = oPtr[x];
			ival = iPtr[x];

			if (pval != 0 && ival != 0) {
				xminus = x - 1;
				xplus = x + 1;

				// 4 connected
				if ((oPtrMinus[x] == 0) ||
						(oPtrPlus[x] == 0) ||
						(oPtr[xplus] == 0) ||
						(oPtr[xminus] == 0)) {
					xQ.push(x);
					yQ.push(y);
					++count;
					continue;
				}

				// 8 connected

				if (connectivity == 8) {
					if ((oPtrMinus[xminus] == 0) ||
						(oPtrMinus[xplus] == 0) ||
						(oPtrPlus[xminus] == 0) ||
						(oPtrPlus[xplus] == 0)) {
								xQ.push(x);
								yQ.push(y);
								++count;
								continue;
					}
				}
			}
		}
	}

//	uint64_t t2 = cci::common::event::timestampInUS();
	//std::cout << "    scan time = " << t2-t1 << "ms for " << count << " queued "<< std::endl;


	// now process the queue.
//	T qval;
	T outval = std::numeric_limits<T>::max();
	int x, y;
	count = 0;
	while (!(xQ.empty())) {
		++count;
		x = xQ.front();
		y = yQ.front();
		xQ.pop();
		yQ.pop();
		xminus = x-1;
		yminus = y-1;
		yplus = y+1;
		xplus = x+1;

		oPtr = output.ptr<T>(y);
		oPtrMinus = output.ptr<T>(y-1);
		oPtrPlus = output.ptr<T>(y+1);
		iPtr = input.ptr<T>(y);
		iPtrMinus = input.ptr<T>(y-1);
		iPtrPlus = input.ptr<T>(y+1);

		// look at the 4 connected components
		if (y > 0) {
			propagateBinary<T>(input, output, xQ, yQ, x, yminus, iPtrMinus, oPtrMinus, outval);
		}
		if (y < maxy) {
			propagateBinary<T>(input, output, xQ, yQ, x, yplus, iPtrPlus, oPtrPlus, outval);
		}
		if (x > 0) {
			propagateBinary<T>(input, output, xQ, yQ, xminus, y, iPtr, oPtr, outval);
		}
		if (x < maxx) {
			propagateBinary<T>(input, output, xQ, yQ, xplus, y, iPtr, oPtr, outval);
		}

		// now 8 connected
		if (connectivity == 8) {

			if (y > 0) {
				if (x > 0) {
					propagateBinary<T>(input, output, xQ, yQ, xminus, yminus, iPtrMinus, oPtrMinus, outval);
				}
				if (x < maxx) {
					propagateBinary<T>(input, output, xQ, yQ, xplus, yminus, iPtrMinus, oPtrMinus, outval);
				}

			}
			if (y < maxy) {
				if (x > 0) {
					propagateBinary<T>(input, output, xQ, yQ, xminus, yplus, iPtrPlus, oPtrPlus,outval);
				}
				if (x < maxx) {
					propagateBinary<T>(input, output, xQ, yQ, xplus, yplus, iPtrPlus, oPtrPlus,outval);
				}

			}
		}

	}

//	uint64_t t3 = cci::common::event::timestampInUS();
	//std::cout << "    queue time = " << t3-t2 << "ms for " << count << " queued" << std::endl;

	return output(Range(1, maxy), Range(1, maxx));
}
template DllExport Mat imreconstructBinary<unsigned char>(const Mat& seeds, const Mat& binaryImage, int connectivity);


template <typename T>
Mat bwselect(const Mat& binaryImage, const Mat& seeds, int connectivity) {
	CV_Assert(binaryImage.channels() == 1);
	CV_Assert(seeds.channels() == 1);
	// only works for binary images.  ~I and max-I are the same....

	/** adopted from bwselect and imfill
	 * bwselet:
	 * seed_indices = sub2ind(size(BW), r(:), c(:));
		BW2 = imfill(~BW, seed_indices, n);
		BW2 = BW2 & BW;
	 *
	 * imfill:
	 * see imfill function.
	 */

	Mat marker = Mat::zeros(seeds.size(), seeds.type());
	binaryImage.copyTo(marker, seeds);

	marker = imreconstructBinary<T>(marker, binaryImage, connectivity);

	return marker & binaryImage;
}

Mat getRBC(const std::vector<Mat>& bgr,  double T1, double T2,
		::cciutils::SimpleCSVLogger *logger=NULL, 
		::cciutils::cv::IntermediateResultHandler *iresHandler=NULL) {
	CV_Assert(bgr.size() == 3);
	/*
	%T1=2.5; T2=2;
    T1=5; T2=4;

	imR2G = double(r)./(double(g)+eps);
    bw1 = imR2G > T1;
    bw2 = imR2G > T2;
    ind = find(bw1);
    if ~isempty(ind)
        [rows, cols]=ind2sub(size(imR2G),ind);
        rbc = bwselect(bw2,cols,rows,8) & (double(r)./(double(b)+eps)>1);
    else
        rbc = zeros(size(imR2G));
    end
	 */
	std::cout.precision(5);
//	double T1 = 5.0;
//	double T2 = 4.0;
	Size s = bgr[0].size();
	Mat bd(s, CV_32FC1);
	Mat gd(s, bd.type());
	Mat rd(s, bd.type());

	bgr[0].convertTo(bd, bd.type(), 1.0, FLT_EPSILON);
	bgr[1].convertTo(gd, gd.type(), 1.0, FLT_EPSILON);
	bgr[2].convertTo(rd, rd.type(), 1.0, 0.0);

	Mat imR2G = rd / gd;
	Mat imR2B = (rd / bd) > 1.0;

	if (iresHandler) iresHandler->saveIntermediate(imR2G, 101);
	if (iresHandler) iresHandler->saveIntermediate(imR2B, 102);

	Mat bw1 = imR2G > T1;
	Mat bw2 = imR2G > T2;
	Mat rbc;
	if (countNonZero(bw1) > 0) {
//		imwrite("test/in-bwselect-marker.pgm", bw2);
//		imwrite("test/in-bwselect-mask.pgm", bw1);
		rbc = bwselect<unsigned char>(bw2, bw1, 8) & imR2B;
	} else {
		rbc = Mat::zeros(bw2.size(), bw2.type());
	}

	return rbc;
}

template <typename T>
Mat morphOpen(const Mat& image, const Mat& kernel) {
	CV_Assert(kernel.rows == kernel.cols);
	CV_Assert(kernel.rows > 1);
	CV_Assert((kernel.rows % 2) == 1);

	int bw = (kernel.rows - 1) / 2;

	// can't use morphologyEx.  the erode phase is not creating a border even though the method signature makes it appear that way.
	// because of this, and the fact that erode and dilate need different border values, have to do the erode and dilate myself.
	//	morphologyEx(image, seg_open, CV_MOP_OPEN, disk3, Point(1,1)); //, Point(-1, -1), 1, BORDER_REFLECT);
	Mat t_image;

	copyMakeBorder(image, t_image, bw, bw, bw, bw, BORDER_CONSTANT, std::numeric_limits<unsigned char>::max());
//	if (bw > 1)	imwrite("test-input-cpu.ppm", t_image);
	Mat t_erode = Mat::zeros(t_image.size(), t_image.type());
	erode(t_image, t_erode, kernel);
//	if (bw > 1) imwrite("test-erode-cpu.ppm", t_erode);

	Mat erode_roi = t_erode(Rect(bw, bw, image.cols, image.rows));
	Mat t_erode2;
	copyMakeBorder(erode_roi,t_erode2, bw, bw, bw, bw, BORDER_CONSTANT, std::numeric_limits<unsigned char>::min());
//	if (bw > 1)	imwrite("test-input2-cpu.ppm", t_erode2);
	Mat t_open = Mat::zeros(t_erode2.size(), t_erode2.type());
	dilate(t_erode2, t_open, kernel);
//	if (bw > 1) imwrite("test-open-cpu.ppm", t_open);
	Mat open = t_open(Rect(bw, bw,image.cols, image.rows));

	t_open.release();
	t_erode2.release();
	erode_roi.release();
	t_erode.release();

	return open;
}
template DllExport Mat morphOpen<unsigned char>(const Mat& image, const Mat& kernel);

template <typename T>
inline void propagate(const Mat& image, Mat& output, std::queue<int>& xQ, std::queue<int>& yQ,
		int x, int y, T* iPtr, T* oPtr, const T& pval) {

	T qval = oPtr[x];
	T ival = iPtr[x];
	if ((qval < pval) && (ival != qval)) {
		oPtr[x] = min(pval, ival);
		xQ.push(x);
		yQ.push(y);
	}
}

template <typename T>
Mat imreconstruct(const Mat& seeds, const Mat& image, int connectivity) {
	CV_Assert(image.channels() == 1);
	CV_Assert(seeds.channels() == 1);


	Mat output(seeds.size() + Size(2,2), seeds.type());
	copyMakeBorder(seeds, output, 1, 1, 1, 1, BORDER_CONSTANT, 0);
	Mat input(image.size() + Size(2,2), image.type());
	copyMakeBorder(image, input, 1, 1, 1, 1, BORDER_CONSTANT, 0);

	T pval, preval;
	int xminus, xplus, yminus, yplus;
	int maxx = output.cols - 1;
	int maxy = output.rows - 1;
	std::queue<int> xQ;
	std::queue<int> yQ;
	T* oPtr;
	T* oPtrMinus;
	T* oPtrPlus;
	T* iPtr;
	T* iPtrPlus;
	T* iPtrMinus;

//	uint64_t t1 = cci::common::event::timestampInUS();

	// raster scan
	for (int y = 1; y < maxy; ++y) {

		oPtr = output.ptr<T>(y);
		oPtrMinus = output.ptr<T>(y-1);
		iPtr = input.ptr<T>(y);

		preval = oPtr[0];
		for (int x = 1; x < maxx; ++x) {
			xminus = x-1;
			xplus = x+1;
			pval = oPtr[x];

			// walk through the neighbor pixels, left and up (N+(p)) only
			pval = max(pval, max(preval, oPtrMinus[x]));

			if (connectivity == 8) {
				pval = max(pval, max(oPtrMinus[xplus], oPtrMinus[xminus]));
			}
			preval = min(pval, iPtr[x]);
			oPtr[x] = preval;
		}
	}

	// anti-raster scan
	int count = 0;
	for (int y = maxy-1; y > 0; --y) {
		oPtr = output.ptr<T>(y);
		oPtrPlus = output.ptr<T>(y+1);
		oPtrMinus = output.ptr<T>(y-1);
		iPtr = input.ptr<T>(y);
		iPtrPlus = input.ptr<T>(y+1);

		preval = oPtr[maxx];
		for (int x = maxx-1; x > 0; --x) {
			xminus = x-1;
			xplus = x+1;

			pval = oPtr[x];

			// walk through the neighbor pixels, right and down (N-(p)) only
			pval = max(pval, max(preval, oPtrPlus[x]));

			if (connectivity == 8) {
				pval = max(pval, max(oPtrPlus[xplus], oPtrPlus[xminus]));
			}

			preval = min(pval, iPtr[x]);
			oPtr[x] = preval;

			// capture the seeds
			// walk through the neighbor pixels, right and down (N-(p)) only
			pval = oPtr[x];

			if ((oPtr[xplus] < min(pval, iPtr[xplus])) ||
					(oPtrPlus[x] < min(pval, iPtrPlus[x]))) {
				xQ.push(x);
				yQ.push(y);
				++count;
				continue;
			}

			if (connectivity == 8) {
				if ((oPtrPlus[xplus] < min(pval, iPtrPlus[xplus])) ||
						(oPtrPlus[xminus] < min(pval, iPtrPlus[xminus]))) {
					xQ.push(x);
					yQ.push(y);
					++count;
					continue;
				}
			}
		}
	}

//	uint64_t t2 = cci::common::event::timestampInUS();
//	std::cout << "    scan time = " << t2-t1 << "ms for " << count << " queue entries."<< std::endl;

	// now process the queue.
//	T qval, ival;
	int x, y;
	count = 0;
	while (!(xQ.empty())) {
		++count;
		x = xQ.front();
		y = yQ.front();
		xQ.pop();
		yQ.pop();
		xminus = x-1;
		xplus = x+1;
		yminus = y-1;
		yplus = y+1;

		oPtr = output.ptr<T>(y);
		oPtrPlus = output.ptr<T>(yplus);
		oPtrMinus = output.ptr<T>(yminus);
		iPtr = input.ptr<T>(y);
		iPtrPlus = input.ptr<T>(yplus);
		iPtrMinus = input.ptr<T>(yminus);

		pval = oPtr[x];

		// look at the 4 connected components
		if (y > 0) {
			propagate<T>(input, output, xQ, yQ, x, yminus, iPtrMinus, oPtrMinus, pval);
		}
		if (y < maxy) {
			propagate<T>(input, output, xQ, yQ, x, yplus, iPtrPlus, oPtrPlus,pval);
		}
		if (x > 0) {
			propagate<T>(input, output, xQ, yQ, xminus, y, iPtr, oPtr,pval);
		}
		if (x < maxx) {
			propagate<T>(input, output, xQ, yQ, xplus, y, iPtr, oPtr,pval);
		}

		// now 8 connected
		if (connectivity == 8) {

			if (y > 0) {
				if (x > 0) {
					propagate<T>(input, output, xQ, yQ, xminus, yminus, iPtrMinus, oPtrMinus, pval);
				}
				if (x < maxx) {
					propagate<T>(input, output, xQ, yQ, xplus, yminus, iPtrMinus, oPtrMinus, pval);
				}

			}
			if (y < maxy) {
				if (x > 0) {
					propagate<T>(input, output, xQ, yQ, xminus, yplus, iPtrPlus, oPtrPlus,pval);
				}
				if (x < maxx) {
					propagate<T>(input, output, xQ, yQ, xplus, yplus, iPtrPlus, oPtrPlus,pval);
				}

			}
		}
	}


//	uint64_t t3 = cci::common::event::timestampInUS();
//	std::cout << "    queue time = " << t3-t2 << "ms for " << count << " queue entries "<< std::endl;

//	std::cout <<  count << " queue entries "<< std::endl;

	return output(Range(1, maxy), Range(1, maxx));

}

template <typename T>
Mat imfillHoles(const Mat& image, bool binary, int connectivity) {
	CV_Assert(image.channels() == 1);

	/* MatLAB imfill hole code:
    if islogical(I)
        mask = uint8(I);
    else
        mask = I;
    end
    mask = padarray(mask, ones(1,ndims(mask)), -Inf, 'both');

    marker = mask;
    idx = cell(1,ndims(I));
    for k = 1:ndims(I)
        idx{k} = 2:(size(marker,k) - 1);
    end
    marker(idx{:}) = Inf;

    mask = imcomplement(mask);
    marker = imcomplement(marker);
    I2 = imreconstruct(marker, mask, conn);
    I2 = imcomplement(I2);
    I2 = I2(idx{:});

    if islogical(I)
        I2 = I2 ~= 0;
    end
	 */

	T mn = cci::common::type::min<T>();
	T mx = std::numeric_limits<T>::max();
	Rect roi = Rect(1, 1, image.cols, image.rows);

	// copy the input and pad with -inf.
	Mat mask(image.size() + Size(2,2), image.type());
	copyMakeBorder(image, mask, 1, 1, 1, 1, BORDER_CONSTANT, mn);
	// create marker with inf inside and -inf at border, and take its complement
	Mat marker;
	Mat marker2(image.size(), image.type(), Scalar(mn));
	// them make the border - OpenCV does not replicate the values when one Mat is a region of another.
	copyMakeBorder(marker2, marker, 1, 1, 1, 1, BORDER_CONSTANT, mx);

	// now do the work...
	mask = nscale::PixelOperations::invert<T>(mask);

//	uint64_t t1 = cci::common::event::timestampInUS();
	Mat output;
	if (binary == true) {
//		imwrite("in-imrecon-binary-marker.pgm", marker);
//		imwrite("in-imrecon-binary-mask.pgm", mask);


//		imwrite("test/in-fillholes-bin-marker.pgm", marker);
//		imwrite("test/in-fillholes-bin-mask.pgm", mask);
		output = imreconstructBinary<T>(marker, mask, connectivity);
	} else {
//		imwrite("test/in-fillholes-gray-marker.pgm", marker);
//		imwrite("test/in-fillholes-gray-mask.pgm", mask);
		output = imreconstruct<T>(marker, mask, connectivity);
	}
//	uint64_t t2 = cci::common::event::timestampInUS();
	//TODO: TEMP std::cout << "    imfill hole imrecon took " << t2-t1 << "ms" << std::endl;

	output = nscale::PixelOperations::invert<T>(output);

	return output(roi);
}
template DllExport Mat imfillHoles<unsigned char>(const Mat& image, bool binary, int connectivity);
template DllExport Mat imfillHoles<int>(const Mat& image, bool binary, int connectivity);

// inclusive min, exclusive max
Mat bwareaopen2(const Mat& image, bool labeled, bool flatten, int minSize, int maxSize, int connectivity, int& count) {
	// only works for binary images.
	CV_Assert(image.channels() == 1);
	// only works for binary images.
	if (labeled == false)
		CV_Assert(image.type() == CV_8U);
	else
		CV_Assert(image.type() == CV_32S);

	//copy, to make data continuous.
	Mat input = Mat::zeros(image.size(), image.type());
	image.copyTo(input);
	Mat_<int> output = Mat_<int>::zeros(input.size());

	::nscale::ConnComponents cc;
	if (labeled == false) {
		Mat_<int> temp = Mat_<int>::zeros(input.size());
		cc.label((unsigned char*)input.data, input.cols, input.rows, (int *)temp.data, -1, connectivity);
		count = cc.areaThresholdLabeled((int *)temp.data, temp.cols, temp.rows, (int *)output.data, -1, minSize, maxSize);
		temp.release();
	} else {
		count = cc.areaThresholdLabeled((int *)input.data, input.cols, input.rows, (int *)output.data, -1, minSize, maxSize);
	}

	input.release();
	if (flatten == true) {
		Mat O2 = Mat::zeros(output.size(), CV_8U);
		O2 = output > -1;
		output.release();
		return O2;
	} else
		return output;

}

int plFindNucleusCandidates(const Mat& img, Mat& seg_norbc, unsigned char blue, 
		unsigned char green, unsigned char red, double T1, double T2, 
		unsigned char G1, int minSize, int maxSize, unsigned char G2, 
		int fillHolesConnectivity, int reconConnectivity,
		::cciutils::SimpleCSVLogger *logger=NULL, 
		::cciutils::cv::IntermediateResultHandler *iresHandler=NULL) {

	uint64_t t0 = cci::common::event::timestampInUS();

	std::vector<Mat> bgr;
	split(img, bgr);
	if (logger) logger->logTimeSinceLastLog("toRGB");

	Mat background = ::nscale::HistologicalEntities::getBackground(bgr, blue, 
		green, red,logger, iresHandler);

	int bgArea = countNonZero(background);
	float ratio = (float)bgArea / (float)(img.size().area());
	if (logger) logger->log("backgroundRatio", ratio);

	if (ratio >= 0.99) {
		//TODO: TEMP std::cout << "background.  next." << std::endl;
		if (logger) logger->logTimeSinceLastLog("background");
		return ::nscale::HistologicalEntities::BACKGROUND;
	} else if (ratio >= 0.9) {
		//TODO: TEMP std::cout << "background.  next." << std::endl;
		if (logger) logger->logTimeSinceLastLog("background likely");
		return ::nscale::HistologicalEntities::BACKGROUND_LIKELY;
	}
	if (logger) logger->logTimeSinceLastLog("background");
	if (iresHandler) iresHandler->saveIntermediate(background, 1);

	Mat rbc = getRBC(bgr, T1, T2, logger, 
		iresHandler);
	if (logger) logger->logTimeSinceLastLog("RBC");
	int rbcPixelCount = countNonZero(rbc);
	if (logger) logger->log("RBCPixCount", rbcPixelCount);

//	imwrite("test/out-rbc.pbm", rbc);
	if (iresHandler) iresHandler->saveIntermediate(rbc, 2);

	/*
	rc = 255 - r;
    rc_open = imopen(rc, strel('disk',10));
    rc_recon = imreconstruct(rc_open,rc);
    diffIm = rc-rc_recon;
	 */

	Mat rc = ::nscale::PixelOperations::invert<unsigned char>(bgr[2]);
	if (logger) logger->logTimeSinceLastLog("invert");

	uint64_t t1 = cci::common::event::timestampInUS();
//	std::cout << "RBC detection: " << t1-t0 << std::endl; 

	Mat rc_open(rc.size(), rc.type());
	//Mat disk19 = getStructuringElement(MORPH_ELLIPSE, Size(19,19));
	// (for 4, 6, and 8 connected, they are approximations).
	unsigned char disk19raw[361] = {
			0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
			0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
			0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
			0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
			0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
			0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
			0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0};
	std::vector<unsigned char> disk19vec(disk19raw, disk19raw+361);
	Mat disk19(disk19vec);
	disk19 = disk19.reshape(1, 19);
	rc_open = morphOpen<unsigned char>(rc, disk19);
//	morphologyEx(rc, rc_open, CV_MOP_OPEN, disk19, Point(-1, -1), 1);
	if (logger) logger->logTimeSinceLastLog("open19");
	if (iresHandler) iresHandler->saveIntermediate(rc_open, 3);

	uint64_t t2 = cci::common::event::timestampInUS();
//	std::cout << "Morph Open: " << t2-t1 << std::endl; 

// for generating test data 
//	imwrite("in-imrecon-gray-marker.pgm", rc_open);
//	imwrite("in-imrecon-gray-mask.pgm", rc);
// END  generating test data

	Mat rc_recon = imreconstruct<unsigned char>(rc_open, rc, 
		reconConnectivity);
	if (iresHandler) iresHandler->saveIntermediate(rc_recon, 4);


	Mat diffIm = rc - rc_recon;
//	imwrite("test/out-redchannelvalleys.ppm", diffIm);
	if (logger) logger->logTimeSinceLastLog("reconToNuclei");
	int rc_openPixelCount = countNonZero(rc_open);
	if (logger) logger->log("rc_openPixCount", rc_openPixelCount);
	if (iresHandler) iresHandler->saveIntermediate(diffIm, 5);
/*
    G1=80; G2=45; % default settings
    %G1=80; G2=30;  % 2nd run

    bw1 = imfill(diffIm>G1,'holes');
 *
 */
	// it is now a parameter
	//unsigned char G1 = 80;
	Mat diffIm2 = diffIm > G1;
	if (logger) logger->logTimeSinceLastLog("threshold1");
	if (iresHandler) iresHandler->saveIntermediate(diffIm2, 6);

//	imwrite("in-fillHolesDump.ppm", diffIm2);
	Mat bw1 = imfillHoles<unsigned char>(diffIm2, true, 
		fillHolesConnectivity);
//	imwrite("test/out-rcvalleysfilledholes.ppm", bw1);
	if (logger) logger->logTimeSinceLastLog("fillHoles1");
	if (iresHandler) iresHandler->saveIntermediate(bw1, 7);
	uint64_t t3 = cci::common::event::timestampInUS();
//	std::cout << "ReconToNuclei " << t3-t2 << std::endl; 


//	// TODO: change back
//	return ::nscale::HistologicalEntities::SUCCESS;
/*
 *     %CHANGE
    [L] = bwlabel(bw1, 8);
    stats = regionprops(L, 'Area');
    areas = [stats.Area];

    %CHANGE
    ind = find(areas>10 & areas<1000);
    bw1 = ismember(L,ind);
    bw2 = diffIm>G2;
    ind = find(bw1);

    if isempty(ind)
        return;
    end
 *
 */
	int compcount2;

//#if defined (USE_UF_CCL)
	Mat bw1_t = bwareaopen2(bw1, false, true, minSize, 
		maxSize, 8, compcount2);
//	printf(" cpu compcount 11-1000 = %d\n", compcount2);
//#else
//	Mat bw1_t = ::nscale::bwareaopen<unsigned char>(bw1, 11, 1000, 8, compcount);
//#endif
	if (logger) logger->logTimeSinceLastLog("areaThreshold1");
	bw1.release();
	if (iresHandler) iresHandler->saveIntermediate(bw1_t, 8);
	if (compcount2 == 0) {
		return ::nscale::HistologicalEntities::NO_CANDIDATES_LEFT;
	}


	// It is now a parameter
	//unsigned char G2 = 45;
	Mat bw2 = diffIm > G2;
	if (iresHandler) iresHandler->saveIntermediate(bw2, 9);

	/*
	 *
    [rows,cols] = ind2sub(size(diffIm),ind);
    seg_norbc = bwselect(bw2,cols,rows,8) & ~rbc;
    seg_nohole = imfill(seg_norbc,'holes');
    seg_open = imopen(seg_nohole,strel('disk',1));
	 *
	 */

	seg_norbc = bwselect<unsigned char>(bw2, bw1_t, 8);
	if (iresHandler) iresHandler->saveIntermediate(seg_norbc, 10);
	seg_norbc = seg_norbc & (rbc == 0);
	if (iresHandler) iresHandler->saveIntermediate(seg_norbc, 11);

//	imwrite("test/out-nucleicandidatesnorbc.ppm", seg_norbc);
	if (logger) logger->logTimeSinceLastLog("blobsGt45");


	return ::nscale::HistologicalEntities::CONTINUE;

}

template <typename T>
Mat imhmin(const Mat& image, T h, int connectivity) {
	// only works for intensity images.
	CV_Assert(image.channels() == 1);

	//	IMHMIN(I,H) suppresses all minima in I whose depth is less than h
	// MatLAB implementation:
	/**
	 *
		I = imcomplement(I);
		I2 = imreconstruct(imsubtract(I,h), I, conn);
		I2 = imcomplement(I2);
	 *
	 */
	Mat mask = nscale::PixelOperations::invert<T>(image);
	Mat marker = mask - h;

//	imwrite("in-imrecon-float-marker.exr", marker);
//	imwrite("in-imrecon-float-mask.exr", mask);

	Mat output = imreconstruct<T>(marker, mask, connectivity);
	return nscale::PixelOperations::invert<T>(output);
}
template DllExport Mat imhmin(const Mat& image, unsigned char h, int connectivity);
template DllExport Mat imhmin(const Mat& image, float h, int connectivity);
template DllExport Mat imhmin(const Mat& image, unsigned short int h, int connectivity);

template <typename T>
Mat_<unsigned char> localMaxima(const Mat& image, int connectivity) {
	CV_Assert(image.channels() == 1);

	// use morphologic reconstruction.
	Mat marker = image - 1;
	Mat_<unsigned char> candidates =
			marker < imreconstruct<T>(marker, image, connectivity);
//	candidates marked as 0 because floodfill with mask will fill only 0's
//	return (image - imreconstruct(marker, image, 8)) >= (1 - std::numeric_limits<T>::epsilon());
	//return candidates;

	// now check the candidates
	// first pad the border
	T mn = cci::common::type::min<T>();
	T mx = std::numeric_limits<unsigned char>::max();
	Mat_<unsigned char> output(candidates.size() + Size(2,2));
	copyMakeBorder(candidates, output, 1, 1, 1, 1, BORDER_CONSTANT, mx);
	Mat input(image.size() + Size(2,2), image.type());
	copyMakeBorder(image, input, 1, 1, 1, 1, BORDER_CONSTANT, mn);

	int maxy = input.rows-1;
	int maxx = input.cols-1;
	int xminus, xplus;
	T val;
	T *iPtr, *iPtrMinus, *iPtrPlus;
	unsigned char *oPtr;
	Rect reg(1, 1, image.cols, image.rows);
	Scalar zero(0);
	Scalar smx(mx);
//	Range xrange(1, maxx);
//	Range yrange(1, maxy);
	Mat inputBlock = input(reg);

	// next iterate over image, and set candidates that are non-max to 0 (via floodfill)
	for (int y = 1; y < maxy; ++y) {

		iPtr = input.ptr<T>(y);
		iPtrMinus = input.ptr<T>(y-1);
		iPtrPlus = input.ptr<T>(y+1);
		oPtr = output.ptr<unsigned char>(y);

		for (int x = 1; x < maxx; ++x) {

			// not a candidate, continue.
			if (oPtr[x] > 0) continue;

			xminus = x-1;
			xplus = x+1;

			val = iPtr[x];
			// compare values

			// 4 connected
			if ((val < iPtrMinus[x]) || (val < iPtrPlus[x]) || (val < iPtr[xminus]) || (val < iPtr[xplus])) {
				// flood with type minimum value (only time when the whole image may have mn is if it's flat)
				floodFill(inputBlock, output, cv::Point(xminus, y-1), smx, &reg, zero, zero, FLOODFILL_FIXED_RANGE | FLOODFILL_MASK_ONLY | connectivity);
				continue;
			}

			// 8 connected
			if (connectivity == 8) {
				if ((val < iPtrMinus[xminus]) || (val < iPtrMinus[xplus]) || (val < iPtrPlus[xminus]) || (val < iPtrPlus[xplus])) {
					// flood with type minimum value (only time when the whole image may have mn is if it's flat)
					floodFill(inputBlock, output, cv::Point(xminus, y-1), smx, &reg, zero, zero, FLOODFILL_FIXED_RANGE | FLOODFILL_MASK_ONLY | connectivity);
					continue;
				}
			}

		}
	}
	return output(reg) == 0;  // similar to bitwise not.
}
template DllExport Mat_<unsigned char> localMaxima<float>(const Mat& image, int connectivity);
template DllExport Mat_<unsigned char> localMaxima<unsigned char>(const Mat& image, int connectivity);

template <typename T>
Mat_<unsigned char> localMinima(const Mat& image, int connectivity) {
	// only works for intensity images.
	CV_Assert(image.channels() == 1);

	Mat cimage = nscale::PixelOperations::invert<T>(image);
	return localMaxima<T>(cimage, connectivity);
}
template DllExport Mat_<unsigned char> localMinima<float>(const Mat& image, int connectivity);
template DllExport Mat_<unsigned char> localMinima<unsigned char>(const Mat& image, int connectivity);

Mat_<int> bwlabel2(const Mat& binaryImage, int connectivity, bool relab) {
	CV_Assert(binaryImage.channels() == 1);
	// only works for binary images.
	CV_Assert(binaryImage.type() == CV_8U);

	//copy, to make data continuous.
	Mat input = Mat::zeros(binaryImage.size(), binaryImage.type());
	binaryImage.copyTo(input);

	::nscale::ConnComponents cc;
	Mat_<int> output = Mat_<int>::zeros(input.size());
	cc.label((unsigned char*) input.data, input.cols, input.rows, (int *)output.data, -1, connectivity);

  // Relabel if requested
  /*
  // VAR J IS SET BUT NOT USED.
	int j = 0;
	if (relab == true) {
		j = cc.relabel(output.cols, output.rows, (int *)output.data, -1);
//		printf("%d number of components\n", j);
	}*/

	input.release();

	return output;
}

template <typename T>
Mat border(Mat& img, T background) {

	// SPECIFIC FOR OPEN CV CPU WATERSHED
	CV_Assert(img.channels() == 1);
	CV_Assert(std::numeric_limits<T>::is_integer);

	Mat result(img.size(), img.type());
	T *ptr, *ptrm1, *res;

	for(int y=1; y< img.rows; y++){
		ptr = img.ptr<T>(y);
		ptrm1 = img.ptr<T>(y-1);

		res = result.ptr<T>(y);
		for (int x = 1; x < img.cols - 1; ++x) {
			if (ptrm1[x] == background && ptr[x] != background &&
					((ptrm1[x-1] != background && ptr[x-1] == background) ||
				(ptrm1[x+1] != background && ptr[x+1] == background))) {
				res[x] = background;
			} else {
				res[x] = ptr[x];
			}
		}
	}

	return result;
}

template Mat border<int>(Mat&, int background);
template Mat border<unsigned char>(Mat&, unsigned char background);


Mat_<int> watershed2(const Mat& origImage, const Mat_<float>& image, int connectivity) {
	// only works for intensity images.
	CV_Assert(image.channels() == 1);
	CV_Assert(origImage.channels() == 3);

	/*
	 * MatLAB implementation:
		cc = bwconncomp(imregionalmin(A, conn), conn);
		L = watershed_meyer(A,conn,cc);

	 */
//	long long int t1, t2;
//	t1 = ::cci::common::event::timestampInUS();
	Mat minima = localMinima<float>(image, connectivity);
//	t2 = ::cci::common::event::timestampInUS();
//	printf("    cpu localMinima = %lld\n", t2-t1);

//	t1 = ::cci::common::event::timestampInUS();
	// watershed is sensitive to label values.  need to relabel.
	Mat_<int> labels = bwlabel2(minima, connectivity, true);
//	t2 = ::cci::common::event::timestampInUS();
//	printf("    cpu UF bwlabel2 = %lld\n", t2-t1);


// need borders, else get edges at edge.
	Mat input, temp, output;
	copyMakeBorder(labels, temp, 1, 1, 1, 1, BORDER_CONSTANT, Scalar_<int>(0));
	copyMakeBorder(origImage, input, 1, 1, 1, 1, BORDER_CONSTANT, Scalar(0, 0, 0));

//	t1 = ::cci::common::event::timestampInUS();

		// input: seeds are labeled from 1 to n, with 0 as background or unknown regions
	// output has -1 as borders.
	watershed(input, temp);
//	t2 = ::cci::common::event::timestampInUS();
//	printf("    CPU watershed = %lld\n", t2-t1);

//	t1 = ::cci::common::event::timestampInUS();
	output = border<int>(temp, (int)-1);
//	t2 = ::cci::common::event::timestampInUS();
//	printf("    CPU watershed border fix = %lld\n", t2-t1);

	return output(Rect(1,1, image.cols, image.rows));
}

int plSeparateNuclei(const Mat& img, const Mat& seg_open, Mat& seg_nonoverlap, int minSizePl, int watershedConnectivity,
		::cciutils::SimpleCSVLogger *logger, ::cciutils::cv::IntermediateResultHandler *iresHandler) {
	/*
	 *
	seg_big = imdilate(bwareaopen(seg_open,30),strel('disk',1));
	 */
	// bwareaopen is done as a area threshold.
	int compcount2;
//#if defined (USE_UF_CCL)
	Mat seg_big_t = bwareaopen2(seg_open, false, true, minSizePl, std::numeric_limits<int>::max(), 8, compcount2);
//	printf(" cpu compcount 30-1000 = %d\n", compcount2);

//#else
//	Mat seg_big_t = ::nscale::bwareaopen<unsigned char>(seg_open, 30, std::numeric_limits<int>::max(), 8, compcount2);
//#endif
	if (logger) logger->logTimeSinceLastLog("30To1000");
	if (iresHandler) iresHandler->saveIntermediate(seg_big_t, 14);


	Mat disk3 = getStructuringElement(MORPH_ELLIPSE, Size(3,3));

	Mat seg_big = Mat::zeros(seg_big_t.size(), seg_big_t.type());
	dilate(seg_big_t, seg_big, disk3);
	if (iresHandler) iresHandler->saveIntermediate(seg_big, 15);

//	imwrite("test/out-nucleicandidatesbig.ppm", seg_big);
	if (logger) logger->logTimeSinceLastLog("dilate");

	/*
	 *
		distance = -bwdist(~seg_big);
		distance(~seg_big) = -Inf;
		distance2 = imhmin(distance, 1);
		 *
	 *
	 */
	// distance transform:  matlab code is doing this:
	// invert the image so nuclei candidates are holes
	// compute the distance (distance of nuclei pixels to background)
	// negate the distance.  so now background is still 0, but nuclei pixels have negative distances
	// set background to -inf

	// really just want the distance map.  CV computes distance to 0.
	// background is 0 in output.
	// then invert to create basins
	Mat dist(seg_big.size(), CV_32FC1);

//	imwrite("seg_big.pbm", seg_big);

	// opencv: compute the distance to nearest zero
	// matlab: compute the distance to the nearest non-zero
	distanceTransform(seg_big, dist, CV_DIST_L2, CV_DIST_MASK_PRECISE);
	double mmin, mmax;
	minMaxLoc(dist, &mmin, &mmax);
	if (iresHandler) iresHandler->saveIntermediate(dist, 16);

	// invert and shift (make sure it's still positive)
	//dist = (mmax + 1.0) - dist;
	dist = - dist;  // appears to work better this way.

//	cciutils::cv::imwriteRaw("test/out-dist", dist);

	// then set the background to -inf and do imhmin
	//Mat distance = Mat::zeros(dist.size(), dist.type());
	// appears to work better with -inf as background
	Mat distance(dist.size(), dist.type(), -std::numeric_limits<float>::max());
	dist.copyTo(distance, seg_big);
//	cciutils::cv::imwriteRaw("test/out-distance", distance);
	if (logger) logger->logTimeSinceLastLog("distTransform");
	if (iresHandler) iresHandler->saveIntermediate(distance, 17);



	// then do imhmin. (prevents small regions inside bigger regions)
//	imwrite("in-imhmin.ppm", distance);

	Mat distance2 = imhmin<float>(distance, 1.0f, 8);
	if (logger) logger->logTimeSinceLastLog("imhmin");
	if (iresHandler) iresHandler->saveIntermediate(distance2, 18);


//	imwrite("distance2.ppm", dist);
//cciutils::cv::imwriteRaw("test/out-distanceimhmin", distance2);


	/*
	 *
		seg_big(watershed(distance2)==0) = 0;
		seg_nonoverlap = seg_big;
     *
	 */
	Mat nuclei = Mat::zeros(img.size(), img.type());
//	Mat distance3 = distance2 + (mmax + 1.0);
//	Mat dist4 = Mat::zeros(distance3.size(), distance3.type());
//	distance3.copyTo(dist4, seg_big);
//	Mat dist5(dist4.size(), CV_8U);
//	dist4.convertTo(dist5, CV_8U, (std::numeric_limits<unsigned char>::max() / mmax));
//	cvtColor(dist5, nuclei, CV_GRAY2BGR);
	img.copyTo(nuclei, seg_big);
	if (logger) logger->logTimeSinceLastLog("nucleiCopy");
	if (iresHandler) iresHandler->saveIntermediate(nuclei, 19);

	// watershed in openCV requires labels.  input foreground > 0, 0 is background
	// critical to use just the nuclei and not the whole image - else get a ring surrounding the regions.
//#if defined (USE_UF_CCL)
	Mat watermask = watershed2(nuclei, distance2, watershedConnectivity);
//#else
//	Mat watermask = ::nscale::watershed(nuclei, distance2, 8);
//#endif
//	cciutils::cv::imwriteRaw("test/out-watershed", watermask);
	if (logger) logger->logTimeSinceLastLog("watershed");
	if (iresHandler) iresHandler->saveIntermediate(watermask, 20);

// MASK approach
	seg_nonoverlap = Mat::zeros(seg_big.size(), seg_big.type());
	seg_big.copyTo(seg_nonoverlap, (watermask >= 0));
	if (logger) logger->logTimeSinceLastLog("water to mask");
	if (iresHandler) iresHandler->saveIntermediate(seg_nonoverlap, 21);
std::cout << "qqqqqqqqqqqqqqqqqqqqqqqqqqqqq"<< std::endl;
//// ERODE has been replaced with border finding and moved into watershed.
//	Mat twm(seg_nonoverlap.rows + 2, seg_nonoverlap.cols + 2, seg_nonoverlap.type());
//	Mat t_nonoverlap = Mat::zeros(twm.size(), twm.type());
//	//ERODE to fix border
//	if (seg_nonoverlap.type() == CV_32S)
//		copyMakeBorder(seg_nonoverlap, twm, 1, 1, 1, 1, BORDER_CONSTANT, Scalar_<int>(std::numeric_limits<int>::max()));
//	else
//		copyMakeBorder(seg_nonoverlap, twm, 1, 1, 1, 1, BORDER_CONSTANT, Scalar_<unsigned char>(std::numeric_limits<unsigned char>::max()));
//	erode(twm, t_nonoverlap, disk3);
//	seg_nonoverlap = t_nonoverlap(Rect(1,1,seg_nonoverlap.cols, seg_nonoverlap.rows));
//	if (logger) logger->logTimeSinceLastLog("watershed erode");
//	if (iresHandler) iresHandler->saveIntermediate(seg_nonoverlap, 22);
//	twm.release();
//	t_nonoverlap.release();

	// LABEL approach -erode does not support 32S
//	seg_nonoverlap = ::nscale::PixelOperations::replace<int>(watermask, (int)0, (int)-1);
	// watershed output: 0 for background, -1 for border.  bwlabel before watershd has 0 for background

	return ::nscale::HistologicalEntities::CONTINUE;

}

int segmentNuclei(const Mat& img, Mat& output, unsigned char blue, 
	unsigned char green, unsigned char red, double T1, double T2, 
	unsigned char G1, int minSize, int maxSize, unsigned char G2, int minSizePl, 
	int minSizeSeg, int maxSizeSeg,  int fillHolesConnectivity, 
	int reconConnectivity, int watershedConnectivity,
	::cciutils::SimpleCSVLogger *logger, 
	::cciutils::cv::IntermediateResultHandler *iresHandler) {

	// image in BGR format
	if (!img.data) return ::nscale::HistologicalEntities::INVALID_IMAGE;

	if (logger) logger->logT0("start");
	if (iresHandler) iresHandler->saveIntermediate(img, 0);

	Mat seg_norbc;
	int findCandidateResult = plFindNucleusCandidates(img, seg_norbc, blue, 
		green, red, T1, T2, G1, minSize, maxSize, G2,  fillHolesConnectivity, 
		reconConnectivity, logger, iresHandler);
	if (findCandidateResult != ::nscale::HistologicalEntities::CONTINUE) {
		return findCandidateResult;
	}
std::cout << "rrrrrrrrrrrrrrrrrrrrrrrrrr"<< std::endl;

	Mat seg_nohole = imfillHoles<unsigned char>(seg_norbc, true, 4);
	if (logger) logger->logTimeSinceLastLog("fillHoles2");
	if (iresHandler) iresHandler->saveIntermediate(seg_nohole, 12);

	Mat disk3 = getStructuringElement(MORPH_ELLIPSE, Size(3,3));
	Mat seg_open = morphOpen<unsigned char>(seg_nohole, disk3);
	if (logger) logger->logTimeSinceLastLog("openBlobs");
	if (iresHandler) iresHandler->saveIntermediate(seg_open, 13);


	Mat seg_nonoverlap;
	int sepResult = plSeparateNuclei(img, 
		seg_open, seg_nonoverlap, minSizePl, watershedConnectivity, logger, 
		iresHandler);
	if (sepResult != ::nscale::HistologicalEntities::CONTINUE) {
		return sepResult;
	}
std::cout << "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"<< std::endl;

	int compcount2;
	// MASK approach
	Mat seg = bwareaopen2(seg_nonoverlap, false, true, minSizeSeg, 
		maxSizeSeg, 4, compcount2);

	if (logger) logger->logTimeSinceLastLog("20To1000");
	if (compcount2 == 0) {
		return ::nscale::HistologicalEntities::NO_CANDIDATES_LEFT;
	}
	if (iresHandler) iresHandler->saveIntermediate(seg, 23);

	// MASK approach
	output = imfillHoles<unsigned char>(seg, true, 
		fillHolesConnectivity);
	// LABEL approach - upstream erode does not support 32S
	if (logger) logger->logTimeSinceLastLog("fillHolesLast");
std::cout << "ccccccccccccccccccccccccccccccccc"<< std::endl;
//	if (logger) logger->endSession();

///	// MASK approach
///	output = nscale::bwlabel2(final, 8, true);
///	final.release();
///
///	if (logger) logger->logTimeSinceLastLog("bwlabel2");
///
///	::nscale::ConnComponents cc;
	return ::nscale::HistologicalEntities::SUCCESS;

}
