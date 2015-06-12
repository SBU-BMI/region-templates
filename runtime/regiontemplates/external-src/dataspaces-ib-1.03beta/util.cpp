
#include "util.h"

void args_parser(int argc, char **argv, int& appSize, int &npx, int &npy, int &npz, int &spx, int &spy, int &spz){
	if(argc < 7){
		printf("Wrong number of arguments!\n");
		exit(1);
	}
	appSize = atoi(argv[1]);
	npx = atoi(argv[2]);
	npy = atoi(argv[3]);
	npz = atoi(argv[4]);
	spx = atoi(argv[5]);
	spy = atoi(argv[6]);
	spz = atoi(argv[7]);
}
