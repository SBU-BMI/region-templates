#include <iostream>
#include <ctime>
#include <string>

#include "Halide.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "BlurJIT.h"
#include "distTiling.h"
#include "PriorityQ.h"
#include "autoTiler.h"

// distributed execution:
// * distributed execution with border locality
//  for each pair of leafs on the dense kd-tree
//      send and execute 2 dense on 2 nodes
//      on each execute finish, send results to root node on a daemon
//      on slack, calculate bg tiles
//      on both finish, redistribute border area between themselves
//      execute half border on both nodes
//      merge on next level to calculate border
//      on last level border finish, return borders to root node
//  * can use root node as manager, which only performs transfers 
//      and calculate bg on slack

using namespace std;

int find_arg_pos(string s, int argc, char** argv) {
    for (int i=1; i<argc; i++)
        if (string(argv[i]).compare(s)==0)
            return i;
    return -1;
}

int main(int argc, char *argv[]) {
    
    if (argc < 2) {
        cout << "usage: tilerTest -d <N> -i <input image>" << endl;
        cout << "\t-d: 0 -> serial execution" << endl;
        cout << "\t    1 -> full distributed execution" << endl;
        return 0;
    }

    // Distribution level
    enum Paral_t {p_serial, p_dist};
    Paral_t paral;
    if (find_arg_pos("-d", argc, argv) == -1) {
        cout << "Missing distribution level." << endl;
        return 0;
    } else
        paral = static_cast<Paral_t>(
            atoi(argv[find_arg_pos("-d", argc, argv)+1]));

    // Input image
    string img_path;
    if (find_arg_pos("-i", argc, argv) == -1) {
        cout << "Missing input image." << endl;
        return 0;
    } else
        img_path = argv[find_arg_pos("-i", argc, argv)+1];
    cv::Mat input = cv::imread(img_path, cv::IMREAD_GRAYSCALE);

    cv::Mat output = cv::Mat::zeros(input.size(), input.type());

    if (paral == p_serial) {
        BlurJIT blur(input, output);
        blur.sched();
        blur.run();
        cv::imwrite("./output.png", output);
    } else if (paral == p_dist) {
        // perform distributed execution
        distExec(argc, argv, input);
    }

    return 0;
}