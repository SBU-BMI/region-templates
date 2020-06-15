#include "fixedGrid.h"

// Brute force algorithm
std::vector<int> factorize(int n) {

    // Using an array for improving access/insert time
    int c=0;
    int fsTmp[(int)ceil(sqrt(n))];
    // fsTmp[c++] = 1;

    // Gets all factors 2
    while (n % 2 == 0) {
        fsTmp[c++] = 2;
        n/=2;
    }  
  
    // Tries all odd numbers after removing 2's factors
    for (int i=3; i <= sqrt(n); i+=2) {
        // Check for divisible
        while (n%i == 0) {
            fsTmp[c++] = i;
            n/=i;
        }
    }
  
    // Checks if the last number is a prime
    if (n > 2)
        fsTmp[c++] = n;

    // Converts array to compact vector
    std::vector<int> fs(fsTmp, fsTmp+c);

    return fs;
}  

// int gcd(int a, int b) {
//     int remainder;
//     while (b != 0) {
//         remainder = a % b;
//         a = b;
//         b = remainder;
//     }
//     return a;
// }

// Pollard's rho based algorithm
// std::vector<int> factorize(int n) {
//     int c;
//     int size;
//     int xFix = 2;
//     long x = 2;
//     std::vector<int> factors;
//     int factor;

//     while (n != 1) {
//         size = 2;
//         do {
//             c = size;
//             do {
//                 x = (x*x+1) % n;
//                 factor = gcd(abs(x - xFix), n);
//                 std::cout << "n-f-x: " << n 
//                     << "-" << factor << "-" << x << std::endl;
//             } while (c>0 && factor==1);
//             size *= 2;
//             xFix = x;
//         } while (factor==1);

//         factors.emplace_back(factor);

//         // If found, add factor to vector
//         if (factor != n) {
//             std::cout << "factor found: " << factor << std::endl;
//             n = n/factor;
//         } else {
//             std::cout << "last factor found: " << factor << std::endl;
//             break;
//         }
//     }

//     return factors;
// }

void intv(std::vector<int>& fs, std::vector<int>& out, int prev, int s, int ii) {
    int f=1;
    if (s > 0) {
        for (int i=ii; i<fs.size(); i++) {
            // std::cout << "[" << s << "]" << (prev*fs[i]) << std::endl;
            out.emplace_back(prev*fs[i]);
            intv(fs, out, prev*fs[i], s-1, i+1);
        }
    }
}

int64_t perim(int xt, int yt, int64_t w, int64_t h) {
    return 2*xt*h + 2*yt*w;
}

int fixedGrid(int64_t nTiles, int64_t w, int64_t h, int64_t mw, int64_t mh, 
        std::list<cv::Rect_<int64_t>>& rois) {
    
    // Gets full factorization of nTiles
    std::vector<int> fs = factorize(nTiles);

    // Gets all sorted combinations of 2 products which results in nTiles
    std::vector<int> out;
    intv(fs, out, 1, fs.size()-1, 0);
    out.emplace_back(1);
    out.emplace_back(nTiles);
    std::sort(out.begin(), out.end());
    
    // Finds best possible combination
    int64_t xTiles = 1;
    int64_t yTiles = nTiles;
    int64_t newPerim;
    int64_t bestPerim = perim(xTiles, yTiles, w, h);
    for (int i=1; i<out.size(); i++) {
        while(out[i]==out[i-1]) i++;
        newPerim = perim(out[i], nTiles/out[i], w, h);
        if (newPerim < bestPerim) {
            xTiles = out[i];
            yTiles = nTiles/out[i];
            bestPerim = newPerim;
        }
    }

    // Updates the tile sizes for the best configuration
    int64_t tw = floor(w/xTiles);
    int64_t th = floor(h/yTiles);

    #ifdef DEBUG
    std::cout << "Full size:" << w << "x" << h << std::endl;
    std::cout << "mw:" << mw << std::endl;
    std::cout << "w:" << w << std::endl;
    std::cout << "mh:" << mh << std::endl;
    std::cout << "h:" << h << std::endl;

    std::cout << "xTiles:" << xTiles << std::endl;
    std::cout << "yTiles:" << yTiles << std::endl;
    std::cout << "tw:" << tw << std::endl;
    std::cout << "th:" << th << std::endl;
    #endif

    // Create regular tiles, except the last line and column
    for (int ti=0; ti<yTiles; ti++) {
        for (int tj=0; tj<xTiles; tj++) {
            // Set the x and y rect coordinates
            int tjTmp  = mw + tj*tw;
            int tjjTmp = mw + tj==(xTiles-1)? w-(tj*tw) : tw;
            int tiTmp  = mh + ti*th;
            int tiiTmp = mh + ti==(yTiles-1)? h-(ti*th) : th;

            // Create the roi for the current tile
            cv::Rect_<int64_t> roi(
                tjTmp, tiTmp, tjjTmp, tiiTmp);
            rois.push_back(roi);

            #ifdef DEBUG
            std::cout << "creating regular roi " << roi.x << "+" 
                      << roi.width << "x" << roi.y << "+" 
                      << roi.height << std::endl;
            #endif
        }
    }
}