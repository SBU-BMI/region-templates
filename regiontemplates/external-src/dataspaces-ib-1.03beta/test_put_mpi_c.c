/* C++ Example */
#include <stdio.h>
#include <mpi.h>
#include "util.h" 
extern "C" {
#include "dataspaces.h"
}
int main (int argc, char **argv){
	int rank, size;

	// dataspaces parameters
	int npapp, npx, npy, npz, spx, spy, spz;

	// Startup MPI
	MPI_Init (&argc, &argv);
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);	/* get current process id */
	MPI_Comm_size (MPI_COMM_WORLD, &size);	/* get number of processes */

	args_parser(argc, argv, npapp, npx, npy, npz, spx, spy, spz);
	dspaces_init(npapp, 1);
	rank = dspaces_rank();
	int peers = dspaces_peers();


	dspaces_lock_on_write("lock");

	double rank_d = rank;
	printf( "Data Spaces put process rank:%d; total peers: %d\n", rank, peers );
	dspaces_put("rank", 1, sizeof(double),  rank, 0, 0, rank, 0, 0, &rank_d);

	dspaces_unlock_on_write("lock");
	
	double rank_ret = -1;
	int err = dspaces_get("rank", 1, sizeof(double),  rank, 0, 0, rank, 0, 0, &rank_ret);
	if(err != 0){
		printf("Failed in get operation. Err=%d\n", err);
	}
	printf("Rank:%d Get_ret = %lf region:<%d,%d,%d;%d,%d,%d> err=%d\n", rank, rank_ret, rank, 0,0, rank, 0, 0, err);

	if(rank == 0){
		double *query_res = (double *)malloc(sizeof(double)*2);
		for(int i = 0; i < peers; i++)
			printf("query[%d]:%lf\n", i, query_res[i]);

		int err = dspaces_select("rank", 1, sizeof(double), 0, 0, 0, peers-1, 0, 0, query_res);
		if(err != 0){
			printf("Failed in select operation. Err=%d\n", err);
		}
		int sizeD = sizeof(double);
		printf("Select  region:<%d,%d,%d;%d,%d,%d> err=%d sizeof(double)=%d\n",  0, 0, 0, peers-1, 0, 0, err, sizeD);
		for(int i = 0; i < peers; i++)
			printf("query[%d]:%lf\n", i, query_res[i]);
	}	
	
	dspaces_barrier();
	dspaces_finalize();
	MPI_Finalize();
	return 0;
}
