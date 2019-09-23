#include "cv.hpp"
#include <queue>
#include <iostream>

#include "ConnComponents.h"
#include "Util.h"
#include "FileUtils.h"
#include "HistologicalEntities.h"

using namespace std;

int plFindNucleusCandidates(const cv::Mat& img, cv::Mat& seg_norbc, unsigned char blue, 
        unsigned char green, unsigned char red, double T1, double T2, 
        unsigned char G1, int minSize, int maxSize, unsigned char G2, 
        int fillHolesConnectivity, int reconConnectivity);

int segmentNuclei(const cv::Mat& img, cv::Mat& output, unsigned char blue, 
    unsigned char green, unsigned char red, double T1, double T2, 
    unsigned char G1, int minSize, int maxSize, unsigned char G2, int minSizePl, 
    int minSizeSeg, int maxSizeSeg,  int fillHolesConnectivity, 
    int reconConnectivity, int watershedConnectivity);

int main () {
    // Input parameters
    unsigned char blue = 200;
    unsigned char green = 200;
    unsigned char red = 200;
    double T1 = 1.0;
    double T2 = 2.0;
    unsigned char G1 = 50;
    unsigned char G2 = 100;
    int minSize = 10;
    int maxSize = 100;
    int minSizePl = 10;
    int minSizeSeg = 10;
    int maxSizeSeg = 100;
    int fillHolesConnectivity = 4;
    int reconConnectivity = 4;
    int watershedConnectivity = 4;

    cv::Mat inputImage = cv::imread("tcga60m.tiff",  cv::IMREAD_COLOR);
    cv::Mat outMask;

    cout << "in: " << inputImage.cols << "x" << inputImage.rows << endl;
    cout << "in chan: " << inputImage.channels() << endl;

    plFindNucleusCandidates(inputImage, outMask, blue, 
            green, red, T1, T2, G1, minSize, maxSize, G2, 
            fillHolesConnectivity, reconConnectivity);

    cv::imwrite("outMask.png", outMask);
}


template <typename T>
inline void propagateBinary(const cv::Mat& image, cv::Mat& output, std::queue<int>& xQ, std::queue<int>& yQ,
        int x, int y, T* iPtr, T* oPtr, const T& foreground) {
    if ((oPtr[x] == 0) && (iPtr[x] != 0)) {
        oPtr[x] = foreground;
        xQ.push(x);
        yQ.push(y);
    }
}

template
void propagateBinary(const cv::Mat&, cv::Mat&, std::queue<int>&, std::queue<int>&,
        int, int, unsigned char* iPtr, unsigned char* oPtr, const unsigned char&);
template
void propagateBinary(const cv::Mat&, cv::Mat&, std::queue<int>&, std::queue<int>&,
        int, int, float* iPtr, float* oPtr, const float&);

template <typename T>
cv::Mat imreconstructBinary(const cv::Mat& seeds, const cv::Mat& image, int connectivity) {
    CV_Assert(image.channels() == 1);
    CV_Assert(seeds.channels() == 1);

    cv::Mat output(seeds.size() + cv::Size(2,2), seeds.type());
    copyMakeBorder(seeds, output, 1, 1, 1, 1, cv::BORDER_CONSTANT, 0);
    cv::Mat input(image.size() + cv::Size(2,2), image.type());
    copyMakeBorder(image, input, 1, 1, 1, 1, cv::BORDER_CONSTANT, 0);

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

    //std::cout << "    scan time = " << t2-t1 << "ms for " << count << " queued "<< std::endl;


    // now process the queue.
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

    //std::cout << "    queue time = " << t3-t2 << "ms for " << count << " queued" << std::endl;

    return output(cv::Range(1, maxy), cv::Range(1, maxx));
}
template cv::Mat imreconstructBinary<unsigned char>(const cv::Mat& seeds, const cv::Mat& binaryImage, int connectivity);


template <typename T>
cv::Mat bwselect(const cv::Mat& binaryImage, const cv::Mat& seeds, int connectivity) {
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

    cv::Mat marker = cv::Mat::zeros(seeds.size(), seeds.type());
    binaryImage.copyTo(marker, seeds);

    marker = imreconstructBinary<T>(marker, binaryImage, connectivity);

    return marker & binaryImage;
}

cv::Mat getRBC(const std::vector<cv::Mat>& bgr,  double T1, double T2) {
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
    //  double T1 = 5.0;
    //  double T2 = 4.0;
    cv::Size s = bgr[0].size();
    cv::Mat bd(s, CV_32FC1);
    cv::Mat gd(s, bd.type());
    cv::Mat rd(s, bd.type());

    bgr[0].convertTo(bd, bd.type(), 1.0, FLT_EPSILON);
    bgr[1].convertTo(gd, gd.type(), 1.0, FLT_EPSILON);
    bgr[2].convertTo(rd, rd.type(), 1.0, 0.0);

    cv::Mat imR2G = rd / gd;
    cv::Mat imR2B = (rd / bd) > 1.0;

    cv::Mat bw1 = imR2G > T1;
    cv::Mat bw2 = imR2G > T2;
    cv::Mat rbc;
    if (countNonZero(bw1) > 0) {
    //      imwrite("test/in-bwselect-marker.pgm", bw2);
    //      imwrite("test/in-bwselect-mask.pgm", bw1);
        rbc = bwselect<unsigned char>(bw2, bw1, 8) & imR2B;
    } else {
        rbc = cv::Mat::zeros(bw2.size(), bw2.type());
    }

    return rbc;
}

template <typename T>
cv::Mat morphOpen(const cv::Mat& image, const cv::Mat& kernel) {
    CV_Assert(kernel.rows == kernel.cols);
    CV_Assert(kernel.rows > 1);
    CV_Assert((kernel.rows % 2) == 1);

    int bw = (kernel.rows - 1) / 2;

    // can't use morphologyEx.  the erode phase is not creating a border even though the method signature makes it appear that way.
    // because of this, and the fact that erode and dilate need different border values, have to do the erode and dilate myself.
    //  morphologyEx(image, seg_open, CV_MOP_OPEN, disk3, Point(1,1)); //, Point(-1, -1), 1, BORDER_REFLECT);
    cv::Mat t_image;

    copyMakeBorder(image, t_image, bw, bw, bw, bw, cv::BORDER_CONSTANT, std::numeric_limits<unsigned char>::max());
    //  if (bw > 1) imwrite("test-input-cpu.ppm", t_image);
    cv::Mat t_erode = cv::Mat::zeros(t_image.size(), t_image.type());
    erode(t_image, t_erode, kernel);
    // cv::imwrite("ref/3eroded.png", t_erode);

    cv::Mat erode_roi = t_erode(cv::Rect(bw, bw, image.cols, image.rows));
    cv::Mat t_erode2;
    copyMakeBorder(erode_roi,t_erode2, bw, bw, bw, bw, cv::BORDER_CONSTANT, std::numeric_limits<unsigned char>::min());
    cv::Mat t_open = cv::Mat::zeros(t_erode2.size(), t_erode2.type());
    dilate(t_erode2, t_open, kernel);
    cv::Mat open = t_open(cv::Rect(bw, bw,image.cols, image.rows));

    t_open.release();
    t_erode2.release();
    erode_roi.release();
    t_erode.release();

    return open;
}
template cv::Mat morphOpen<unsigned char>(const cv::Mat& image, const cv::Mat& kernel);

template <typename T>
inline void propagate(const cv::Mat& image, cv::Mat& output, std::queue<int>& xQ, std::queue<int>& yQ,
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
// seeds=rcopen=J, image=rc=I
cv::Mat imreconstruct(const cv::Mat& seeds, const cv::Mat& image, int connectivity) {
    CV_Assert(image.channels() == 1);
    CV_Assert(seeds.channels() == 1);


    cv::Mat output(seeds.size() + cv::Size(2,2), seeds.type());
    copyMakeBorder(seeds, output, 1, 1, 1, 1, cv::BORDER_CONSTANT, 0);
    cv::Mat input(image.size() + cv::Size(2,2), image.type());
    copyMakeBorder(image, input, 1, 1, 1, 1, cv::BORDER_CONSTANT, 0);

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

    // now process the queue.
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

    return output(cv::Range(1, maxy), cv::Range(1, maxx));
}

template<typename T>
cv::Mat invert(const cv::Mat &img) {
    // write the raw image
    CV_Assert(img.channels() == 1);

    if (std::numeric_limits<T>::is_integer) {

        if (std::numeric_limits<T>::is_signed) {
            cv::Mat output;
            bitwise_not(img, output);
            return output + 1;
        } else {
            // unsigned int
            return std::numeric_limits<T>::max() - img;
        }

    } else {
        // floating point type
        return -img;
    }
}
template cv::Mat invert<unsigned char>(const cv::Mat &);
template cv::Mat invert<float>(const cv::Mat &);
template cv::Mat invert<int>(const cv::Mat &);  // for imfillholes
template cv::Mat invert<unsigned short int>(const cv::Mat &);

template <typename T>
cv::Mat imfillHoles(const cv::Mat& image, bool binary, int connectivity) {
    CV_Assert(image.channels() == 1);

    /* cv::MatLAB imfill hole code:
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
    cv::Rect roi = cv::Rect(1, 1, image.cols, image.rows);

    // copy the input and pad with -inf.
    cv::Mat mask(image.size() + cv::Size(2,2), image.type());
    copyMakeBorder(image, mask, 1, 1, 1, 1, cv::BORDER_CONSTANT, mn);
    // create marker with inf inside and -inf at border, and take its complement
    cv::Mat marker;
    cv::Mat marker2(image.size(), image.type(), cv::Scalar(mn));
    // them make the border - OpenCV does not replicate the values when one cv::Mat is a region of another.
    copyMakeBorder(marker2, marker, 1, 1, 1, 1, cv::BORDER_CONSTANT, mx);

    // now do the work...
    mask = invert<T>(mask);
    // imwrite("ref/5invRecon.png", mask);

    cv::Mat output;
    if (binary == true) {
    //      imwrite("in-imrecon-binary-marker.pgm", marker);
    //      imwrite("in-imrecon-binary-mask.pgm", mask);


    //      imwrite("test/in-fillholes-bin-marker.pgm", marker);
    //      imwrite("test/in-fillholes-bin-mask.pgm", mask);
        output = imreconstructBinary<T>(marker, mask, connectivity);
    } else {
    //      imwrite("test/in-fillholes-gray-marker.pgm", marker);
    //      imwrite("test/in-fillholes-gray-mask.pgm", mask);
        output = imreconstruct<T>(marker, mask, connectivity);
    }
    imwrite("ref/5pre_fill2.png", output);
    //  uint64_t t2 = cci::common::event::timestampInUS();
    //TODO: TEMP std::cout << "    imfill hole imrecon took " << t2-t1 << "ms" << std::endl;

    output = invert<T>(output);

    return output(roi);
}
template cv::Mat imfillHoles<unsigned char>(const cv::Mat& image, bool binary, int connectivity);
template cv::Mat imfillHoles<int>(const cv::Mat& image, bool binary, int connectivity);

// inclusive min, exclusive max
cv::Mat bwareaopen2(const cv::Mat& image, bool labeled, bool flatten, int minSize, int maxSize, int connectivity, int& count) {
    // only works for binary images.
    CV_Assert(image.channels() == 1);
    // only works for binary images.
    if (labeled == false)
        CV_Assert(image.type() == CV_8U);
    else
        CV_Assert(image.type() == CV_32S);

    //copy, to make data continuous.
    cv::Mat input = cv::Mat::zeros(image.size(), image.type());
    image.copyTo(input);
    cv::Mat_<int> output = cv::Mat_<int>::zeros(input.size());

    ::nscale::ConnComponents cc;
    if (labeled == false) {
        cv::Mat_<int> temp = cv::Mat_<int>::zeros(input.size());
        cc.label((unsigned char*)input.data, input.cols, input.rows, (int *)temp.data, -1, connectivity);
        count = cc.areaThresholdLabeled((int *)temp.data, temp.cols, temp.rows, (int *)output.data, -1, minSize, maxSize);
        temp.release();
    } else {
        count = cc.areaThresholdLabeled((int *)input.data, input.cols, input.rows, (int *)output.data, -1, minSize, maxSize);
    }

    input.release();
    if (flatten == true) {
        cv::Mat O2 = cv::Mat::zeros(output.size(), CV_8U);
        O2 = output > -1;
        output.release();
        return O2;
    } else
        return output;

}

cv::Mat getBackground(const std::vector<cv::Mat>& bgr, 
    unsigned char blue, unsigned char green, unsigned char red) {

    return (bgr[0] > blue) & (bgr[1] > green) & (bgr[2] > red);
}

cv::Mat getBackground(const cv::Mat& img, unsigned char blue, 
    unsigned char green, unsigned char red) {
    CV_Assert(img.channels() == 3);

    std::vector<cv::Mat> bgr;
    split(img, bgr);
    return getBackground(bgr, blue, green, red);
}

int plFindNucleusCandidates(const cv::Mat& img, cv::Mat& seg_norbc, unsigned char blue, 
        unsigned char green, unsigned char red, double T1, double T2, 
        unsigned char G1, int minSize, int maxSize, unsigned char G2, 
        int fillHolesConnectivity, int reconConnectivity) {

    cout << "[plFindNucleusCandidates] init" << endl;

    uint64_t t0 = cci::common::event::timestampInUS();

    std::vector<cv::Mat> bgr;
    split(img, bgr);

    cv::Mat background = getBackground(bgr, blue, green, red);
    // cv::imwrite("ref/0background.png", background);
    cout << "ref/0background.png" << endl;

    int bgArea = countNonZero(background);
    float ratio = (float)bgArea / (float)(img.size().area());

    cout << "[plFindNucleusCandidates] 1" << endl;
    if (ratio >= 0.99) {
        std::cout << "background.  next." << std::endl;
        return ::nscale::HistologicalEntities::BACKGROUND;
    } else if (ratio >= 0.9) {
        std::cout << "background.  next." << std::endl;
        return ::nscale::HistologicalEntities::BACKGROUND_LIKELY;
    }


    cv::Mat rbc = getRBC(bgr, T1, T2);
    // cv::imwrite("ref/1rbc.png", rbc);
    cout << "ref/1rbc.png" << endl;
    int rbcPixelCount = countNonZero(rbc);

//  imwrite("test/out-rbc.pbm", rbc);
    /*
    rc = 255 - r;
    rc_open = imopen(rc, strel('disk',10));
    rc_recon = imreconstruct(rc_open,rc);
    diffIm = rc-rc_recon;
     */
    cv::Mat rc = invert<unsigned char>(bgr[2]);
    // cv::imwrite("ref/2rc.png", rc);
    cout << "ref/2rc.png" << endl;
    uint64_t t1 = cci::common::event::timestampInUS();
//  std::cout << "RBC detection: " << t1-t0 << std::endl; 

    cv::Mat rc_open(rc.size(), rc.type());
    //cv::Mat disk19 = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(19,19));
    // (for 4, 6, and 8 connected, they are approxicv::Mations).
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
    cv::Mat disk19(disk19vec);
    disk19 = disk19.reshape(1, 19);
    rc_open = morphOpen<unsigned char>(rc, disk19);
    // cv::imwrite("ref/3rc_open.png", rc_open);
//  morphologyEx(rc, rc_open, CV_MOP_OPEN, disk19, Point(-1, -1), 1);

    uint64_t t2 = cci::common::event::timestampInUS();
//  std::cout << "Morph Open: " << t2-t1 << std::endl; 

// for generating test data 
//  imwrite("in-imrecon-gray-marker.pgm", rc_open);
//  imwrite("in-imrecon-gray-mask.pgm", rc);
// END  generating test data

    cv::Mat rc_recon = imreconstruct<unsigned char>(rc_open, rc, 
        reconConnectivity);
    // cv::imwrite("ref/4rc_recon.png", rc_recon);


    cv::Mat diffIm = rc - rc_recon;
//  imwrite("test/out-redchannelvalleys.ppm", diffIm);
    int rc_openPixelCount = countNonZero(rc_open);
/*
    G1=80; G2=45; % default settings
    %G1=80; G2=30;  % 2nd run

    bw1 = imfill(diffIm>G1,'holes');
 *
 */
    // it is now a parameter
    //unsigned char G1 = 80;
    cv::Mat diffIm2 = diffIm > G1;
    // cv::imwrite("ref/5pre_fill.png", diffIm2);

//  imwrite("in-fillHolesDump.ppm", diffIm2);
    cv::Mat bw1 = imfillHoles<unsigned char>(diffIm2, true, 
        fillHolesConnectivity);
    // cv::imwrite("ref/6bw1.png", bw1);
//  imwrite("test/out-rcvalleysfilledholes.ppm", bw1);
    uint64_t t3 = cci::common::event::timestampInUS();
//  std::cout << "ReconToNuclei " << t3-t2 << std::endl; 


//  // TODO: change back
//  return ::nscale::HistologicalEntities::SUCCESS;
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
    cv::Mat bw1_t = bwareaopen2(bw1, false, true, minSize, 
        maxSize, 8, compcount2);
    cv::imwrite("ref/7bw1_t.png", bw1_t);
//  printf(" cpu compcount 11-1000 = %d\n", compcount2);
//#else
//  cv::Mat bw1_t = ::nscale::bwareaopen<unsigned char>(bw1, 11, 1000, 8, compcount);
//#endif
    bw1.release();
    if (compcount2 == 0) {
        return ::nscale::HistologicalEntities::NO_CANDIDATES_LEFT;
    }


    // It is now a parameter
    //unsigned char G2 = 45;
    cv::Mat bw2 = diffIm > G2;

    /*
     *
    [rows,cols] = ind2sub(size(diffIm),ind);
    seg_norbc = bwselect(bw2,cols,rows,8) & ~rbc;
    seg_nohole = imfill(seg_norbc,'holes');
    seg_open = imopen(seg_nohole,strel('disk',1));
     *
     */

    seg_norbc = bwselect<unsigned char>(bw2, bw1_t, 8);
    seg_norbc = seg_norbc & (rbc == 0);
    cv::imwrite("ref/8seg_norbc.png", seg_norbc);

//  imwrite("test/out-nucleicandidatesnorbc.ppm", seg_norbc);


    return ::nscale::HistologicalEntities::CONTINUE;

}

// template <typename T>
// cv::Mat imhmin(const cv::Mat& image, T h, int connectivity) {
//     // only works for intensity images.
//     CV_Assert(image.channels() == 1);

//     //  IMHMIN(I,H) suppresses all minima in I whose depth is less than h
//     // cv::MatLAB implementation:
//     /**
//      *
//         I = imcomplement(I);
//         I2 = imreconstruct(imsubtract(I,h), I, conn);
//         I2 = imcomplement(I2);
//      *
//      */
//     cv::Mat mask = invert<T>(image);
//     cv::Mat marker = mask - h;

// //  imwrite("in-imrecon-float-marker.exr", marker);
// //  imwrite("in-imrecon-float-mask.exr", mask);

//     cv::Mat output = imreconstruct<T>(marker, mask, connectivity);
//     return invert<T>(output);
// }
// template cv::Mat imhmin(const cv::Mat& image, unsigned char h, int connectivity);
// template cv::Mat imhmin(const cv::Mat& image, float h, int connectivity);
// template cv::Mat imhmin(const cv::Mat& image, unsigned short int h, int connectivity);

// template <typename T>
// cv::Mat_<unsigned char> localMaxima(const cv::Mat& image, int connectivity) {
//     CV_Assert(image.channels() == 1);

//     // use morphologic reconstruction.
//     cv::Mat marker = image - 1;
//     cv::Mat_<unsigned char> candidates =
//             marker < imreconstruct<T>(marker, image, connectivity);
// //  candidates marked as 0 because floodfill with mask will fill only 0's
// //  return (image - imreconstruct(marker, image, 8)) >= (1 - std::numeric_limits<T>::epsilon());
//     //return candidates;

//     // now check the candidates
//     // first pad the border
//     T mn = cci::common::type::min<T>();
//     T mx = std::numeric_limits<unsigned char>::max();
//     cv::Mat_<unsigned char> output(candidates.size() + cv::Size(2,2));
//     copyMakeBorder(candidates, output, 1, 1, 1, 1, cv::BORDER_CONSTANT, mx);
//     cv::Mat input(image.size() + cv::Size(2,2), image.type());
//     copyMakeBorder(image, input, 1, 1, 1, 1, cv::BORDER_CONSTANT, mn);

//     int maxy = input.rows-1;
//     int maxx = input.cols-1;
//     int xminus, xplus;
//     T val;
//     T *iPtr, *iPtrMinus, *iPtrPlus;
//     unsigned char *oPtr;
//     cv::Rect reg(1, 1, image.cols, image.rows);
//     cv::Scalar zero(0);
//     cv::Scalar smx(mx);
// //  cv::Range xrange(1, maxx);
// //  cv::Range yrange(1, maxy);
//     cv::Mat inputBlock = input(reg);

//     // next iterate over image, and set candidates that are non-max to 0 (via floodfill)
//     for (int y = 1; y < maxy; ++y) {

//         iPtr = input.ptr<T>(y);
//         iPtrMinus = input.ptr<T>(y-1);
//         iPtrPlus = input.ptr<T>(y+1);
//         oPtr = output.ptr<unsigned char>(y);

//         for (int x = 1; x < maxx; ++x) {

//             // not a candidate, continue.
//             if (oPtr[x] > 0) continue;

//             xminus = x-1;
//             xplus = x+1;

//             val = iPtr[x];
//             // compare values

//             // 4 connected
//             if ((val < iPtrMinus[x]) || (val < iPtrPlus[x]) || (val < iPtr[xminus]) || (val < iPtr[xplus])) {
//                 // flood with type minimum value (only time when the whole image may have mn is if it's flat)
//                 floodFill(inputBlock, output, cv::Point(xminus, y-1), smx, &reg, zero, zero, cv::FLOODFILL_FIXED_RANGE | cv::FLOODFILL_MASK_ONLY | connectivity);
//                 continue;
//             }

//             // 8 connected
//             if (connectivity == 8) {
//                 if ((val < iPtrMinus[xminus]) || (val < iPtrMinus[xplus]) || (val < iPtrPlus[xminus]) || (val < iPtrPlus[xplus])) {
//                     // flood with type minimum value (only time when the whole image may have mn is if it's flat)
//                     floodFill(inputBlock, output, cv::Point(xminus, y-1), smx, &reg, zero, zero, cv::FLOODFILL_FIXED_RANGE | cv::FLOODFILL_MASK_ONLY | connectivity);
//                     continue;
//                 }
//             }

//         }
//     }
//     return output(reg) == 0;  // similar to bitwise not.
// }
// template cv::Mat_<unsigned char> localMaxima<float>(const cv::Mat& image, int connectivity);
// template cv::Mat_<unsigned char> localMaxima<unsigned char>(const cv::Mat& image, int connectivity);

// template <typename T>
// cv::Mat_<unsigned char> localMinima(const cv::Mat& image, int connectivity) {
//     // only works for intensity images.
//     CV_Assert(image.channels() == 1);

//     cv::Mat cimage = invert<T>(image);
//     return localMaxima<T>(cimage, connectivity);
// }
// template cv::Mat_<unsigned char> localMinima<float>(const cv::Mat& image, int connectivity);
// template cv::Mat_<unsigned char> localMinima<unsigned char>(const cv::Mat& image, int connectivity);

// cv::Mat_<int> bwlabel2(const cv::Mat& binaryImage, int connectivity, bool relab) {
//     CV_Assert(binaryImage.channels() == 1);
//     // only works for binary images.
//     CV_Assert(binaryImage.type() == CV_8U);

//     //copy, to make data continuous.
//     cv::Mat input = cv::Mat::zeros(binaryImage.size(), binaryImage.type());
//     binaryImage.copyTo(input);

//     ::nscale::ConnComponents cc;
//     cv::Mat_<int> output = cv::Mat_<int>::zeros(input.size());
//     cc.label((unsigned char*) input.data, input.cols, input.rows, (int *)output.data, -1, connectivity);

//   // Relabel if requested
//   /*
//   // VAR J IS SET BUT NOT USED.
//     int j = 0;
//     if (relab == true) {
//         j = cc.relabel(output.cols, output.rows, (int *)output.data, -1);
// //      printf("%d number of components\n", j);
//     }*/

//     input.release();

//     return output;
// }

// template <typename T>
// cv::Mat border(cv::Mat& img, T background) {

//     // SPECIFIC FOR OPEN CV CPU WATERSHED
//     CV_Assert(img.channels() == 1);
//     CV_Assert(std::numeric_limits<T>::is_integer);

//     cv::Mat result(img.size(), img.type());
//     T *ptr, *ptrm1, *res;

//     for(int y=1; y< img.rows; y++){
//         ptr = img.ptr<T>(y);
//         ptrm1 = img.ptr<T>(y-1);

//         res = result.ptr<T>(y);
//         for (int x = 1; x < img.cols - 1; ++x) {
//             if (ptrm1[x] == background && ptr[x] != background &&
//                     ((ptrm1[x-1] != background && ptr[x-1] == background) ||
//                 (ptrm1[x+1] != background && ptr[x+1] == background))) {
//                 res[x] = background;
//             } else {
//                 res[x] = ptr[x];
//             }
//         }
//     }

//     return result;
// }

// template cv::Mat border<int>(cv::Mat&, int background);
// template cv::Mat border<unsigned char>(cv::Mat&, unsigned char background);


// cv::Mat_<int> watershed2(const cv::Mat& origImage, const cv::Mat_<float>& image, int connectivity) {
//     // only works for intensity images.
//     CV_Assert(image.channels() == 1);
//     CV_Assert(origImage.channels() == 3);

//     /*
//      * cv::MatLAB implementation:
//         cc = bwconncomp(imregionalmin(A, conn), conn);
//         L = watershed_meyer(A,conn,cc);

//      */
// //  long long int t1, t2;
// //  t1 = ::cci::common::event::timestampInUS();
//     cv::Mat minima = localMinima<float>(image, connectivity);
// //  t2 = ::cci::common::event::timestampInUS();
// //  printf("    cpu localMinima = %lld\n", t2-t1);

// //  t1 = ::cci::common::event::timestampInUS();
//     // watershed is sensitive to label values.  need to relabel.
//     cv::Mat_<int> labels = bwlabel2(minima, connectivity, true);
// //  t2 = ::cci::common::event::timestampInUS();
// //  printf("    cpu UF bwlabel2 = %lld\n", t2-t1);


// // need borders, else get edges at edge.
//     cv::Mat input, temp, output;
//     copyMakeBorder(labels, temp, 1, 1, 1, 1, cv::BORDER_CONSTANT, cv::Scalar_<int>(0));
//     copyMakeBorder(origImage, input, 1, 1, 1, 1, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

// //  t1 = ::cci::common::event::timestampInUS();

//         // input: seeds are labeled from 1 to n, with 0 as background or unknown regions
//     // output has -1 as borders.
//     watershed(input, temp);
// //  t2 = ::cci::common::event::timestampInUS();
// //  printf("    CPU watershed = %lld\n", t2-t1);

// //  t1 = ::cci::common::event::timestampInUS();
//     output = border<int>(temp, (int)-1);
// //  t2 = ::cci::common::event::timestampInUS();
// //  printf("    CPU watershed border fix = %lld\n", t2-t1);

//     return output(cv::Rect(1,1, image.cols, image.rows));
// }

// int plSeparateNuclei(const cv::Mat& img, const cv::Mat& seg_open, cv::Mat& seg_nonoverlap, int minSizePl, int watershedConnectivity) {
//     /*
//      *
//     seg_big = imdilate(bwareaopen(seg_open,30),strel('disk',1));
//      */
//     // bwareaopen is done as a area threshold.
//     int compcount2;
// //#if defined (USE_UF_CCL)
//     cv::Mat seg_big_t = bwareaopen2(seg_open, false, true, minSizePl, std::numeric_limits<int>::max(), 8, compcount2);
// //  printf(" cpu compcount 30-1000 = %d\n", compcount2);

// //#else
// //  cv::Mat seg_big_t = ::nscale::bwareaopen<unsigned char>(seg_open, 30, std::numeric_limits<int>::max(), 8, compcount2);
// //#endif

//     cv::Mat disk3 = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3,3));

//     cv::Mat seg_big = cv::Mat::zeros(seg_big_t.size(), seg_big_t.type());
//     dilate(seg_big_t, seg_big, disk3);

// //  imwrite("test/out-nucleicandidatesbig.ppm", seg_big);

//     /*
//      *
//         distance = -bwdist(~seg_big);
//         distance(~seg_big) = -Inf;
//         distance2 = imhmin(distance, 1);
//          *
//      *
//      */
//     // distance transform:  cv::Matlab code is doing this:
//     // invert the image so nuclei candidates are holes
//     // compute the distance (distance of nuclei pixels to background)
//     // negate the distance.  so now background is still 0, but nuclei pixels have negative distances
//     // set background to -inf

//     // really just want the distance map.  CV computes distance to 0.
//     // background is 0 in output.
//     // then invert to create basins
//     cv::Mat dist(seg_big.size(), CV_32FC1);

// //  imwrite("seg_big.pbm", seg_big);

//     // opencv: compute the distance to nearest zero
//     // cv::Matlab: compute the distance to the nearest non-zero
//     distanceTransform(seg_big, dist, CV_DIST_L2, CV_DIST_MASK_PRECISE);
//     double mmin, mmax;
//     minMaxLoc(dist, &mmin, &mmax);

//     // invert and shift (make sure it's still positive)
//     //dist = (mmax + 1.0) - dist;
//     dist = - dist;  // appears to work better this way.

//     // then set the background to -inf and do imhmin
//     //cv::Mat distance = cv::Mat::zeros(dist.size(), dist.type());
//     // appears to work better with -inf as background
//     cv::Mat distance(dist.size(), dist.type(), -std::numeric_limits<float>::max());
//     dist.copyTo(distance, seg_big);



//     // then do imhmin. (prevents small regions inside bigger regions)
// //  imwrite("in-imhmin.ppm", distance);

//     cv::Mat distance2 = imhmin<float>(distance, 1.0f, 8);

// //  imwrite("distance2.ppm", dist);


//     /*
//      *
//         seg_big(watershed(distance2)==0) = 0;
//         seg_nonoverlap = seg_big;
//      *
//      */
//     cv::Mat nuclei = cv::Mat::zeros(img.size(), img.type());
// //  cv::Mat distance3 = distance2 + (mmax + 1.0);
// //  cv::Mat dist4 = cv::Mat::zeros(distance3.size(), distance3.type());
// //  distance3.copyTo(dist4, seg_big);
// //  cv::Mat dist5(dist4.size(), CV_8U);
// //  dist4.convertTo(dist5, CV_8U, (std::numeric_limits<unsigned char>::max() / mmax));
// //  cvtColor(dist5, nuclei, CV_GRAY2BGR);
//     img.copyTo(nuclei, seg_big);

//     // watershed in openCV requires labels.  input foreground > 0, 0 is background
//     // critical to use just the nuclei and not the whole image - else get a ring surrounding the regions.
// //#if defined (USE_UF_CCL)
//     cv::Mat watermask = watershed2(nuclei, distance2, watershedConnectivity);
// //#else
// //  cv::Mat watermask = ::nscale::watershed(nuclei, distance2, 8);
// //#endif

// // MASK approach
//     seg_nonoverlap = cv::Mat::zeros(seg_big.size(), seg_big.type());
//     seg_big.copyTo(seg_nonoverlap, (watermask >= 0));
// //// ERODE has been replaced with border finding and moved into watershed.
// //  cv::Mat twm(seg_nonoverlap.rows + 2, seg_nonoverlap.cols + 2, seg_nonoverlap.type());
// //  cv::Mat t_nonoverlap = cv::Mat::zeros(twm.size(), twm.type());
// //  //ERODE to fix border
// //  if (seg_nonoverlap.type() == CV_32S)
// //      copyMakeBorder(seg_nonoverlap, twm, 1, 1, 1, 1, cv::BORDER_CONSTANT, Scalar_<int>(std::numeric_limits<int>::max()));
// //  else
// //      copyMakeBorder(seg_nonoverlap, twm, 1, 1, 1, 1, cv::BORDER_CONSTANT, Scalar_<unsigned char>(std::numeric_limits<unsigned char>::max()));
// //  erode(twm, t_nonoverlap, disk3);
// //  seg_nonoverlap = t_nonoverlap(cv::Rect(1,1,seg_nonoverlap.cols, seg_nonoverlap.rows));
// //  twm.release();
// //  t_nonoverlap.release();

//     // LABEL approach -erode does not support 32S
// //  seg_nonoverlap = ::nscale::PixelOperations::replace<int>(watermask, (int)0, (int)-1);
//     // watershed output: 0 for background, -1 for border.  bwlabel before watershd has 0 for background

//     return ::nscale::HistologicalEntities::CONTINUE;

// }

// int segmentNuclei(const cv::Mat& img, cv::Mat& output, unsigned char blue, 
//     unsigned char green, unsigned char red, double T1, double T2, 
//     unsigned char G1, int minSize, int maxSize, unsigned char G2, int minSizePl, 
//     int minSizeSeg, int maxSizeSeg,  int fillHolesConnectivity, 
//     int reconConnectivity, int watershedConnectivity) {
//     // image in BGR forcv::Mat
//     if (!img.data) return ::nscale::HistologicalEntities::INVALID_IMAGE;


//     cv::Mat seg_norbc;
//     int findCandidateResult = plFindNucleusCandidates(img, seg_norbc, blue, 
//         green, red, T1, T2, G1, minSize, maxSize, G2,  fillHolesConnectivity, 
//         reconConnectivity);
//     if (findCandidateResult != ::nscale::HistologicalEntities::CONTINUE) {
//         return findCandidateResult;
//     }

//     cv::Mat seg_nohole = imfillHoles<unsigned char>(seg_norbc, true, 4);

//     cv::Mat disk3 = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3,3));
//     cv::Mat seg_open = morphOpen<unsigned char>(seg_nohole, disk3);


//     cv::Mat seg_nonoverlap;
//     int sepResult = plSeparateNuclei(img, 
//         seg_open, seg_nonoverlap, minSizePl, watershedConnectivity);
//     if (sepResult != ::nscale::HistologicalEntities::CONTINUE) {
//         return sepResult;
//     }

//     int compcount2;
//     // MASK approach
//     cv::Mat seg = bwareaopen2(seg_nonoverlap, false, true, minSizeSeg, 
//         maxSizeSeg, 4, compcount2);

//     if (compcount2 == 0) {
//         return ::nscale::HistologicalEntities::NO_CANDIDATES_LEFT;
//     }

//     // MASK approach
//     output = imfillHoles<unsigned char>(seg, true, 
//         fillHolesConnectivity);
//     // LABEL approach - upstream erode does not support 32S

// /// // MASK approach
// /// output = nscale::bwlabel2(final, 8, true);
// /// final.release();
// ///
// ///
// /// ::nscale::ConnComponents cc;
//     return ::nscale::HistologicalEntities::SUCCESS;

// }
