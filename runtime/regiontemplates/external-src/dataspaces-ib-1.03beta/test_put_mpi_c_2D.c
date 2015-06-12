/* C++ Example */
#include <stdio.h>
#include <mpi.h>
#include "util.h" 
#include "assert.h" 

extern "C" {
#include "dataspaces.h"
}

void diff(char *matA, char *matB, int x, int y){
	for(int i =0; i < y; i++){
		for(int c = 0; c < x; c++){
			if(matA[i*x+c] != matB[i*x+c]){
				printf("Matrices differ in x:%d, y:%d; matA:%d, matB:%d\n",c,i,matA[i*x+c], matB[i*x+c]);
				return;
			}
		}
	}
	printf("Matrices are equal!\n");
	return;
}
char *initMatrix(int x, int y){
	assert(x>0 && y>0);
	char *matrix = (char*)malloc(sizeof(char)*x*y);
	if(matrix == NULL){
		printf("FAILED to allocate memory\n");
		exit(1);
	}
	for(int i = 0; i < y; i++){
		for(int c = 0; c < x; c++){
			matrix[i*x+c] = (c+i)%128;
		}
	}
	return matrix;
}

/* ./app npapp npx npy npz spx spy spz timestems
 The arguments mean:
     npapp - number of processors in the application
     np{x,y,z} - number of processors in the * direction (data decomposition)
     sp{x,y,z} - size of the data block in the * direction
     timestep  - number of iterations */

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
	
	printf("spx:%d spy:%d\n", spx, spy);
	char *matrix = initMatrix(spx, spy);
	printf("after allocation\n");	

	dspaces_lock_on_write("lock");

	printf( "Data Spaces put process rank:%d; total peers: %d <%d,%d,%d;%d,%d,%d>\n", rank, peers,rank*spx, 0, 0, ((rank+1)*spx)-1, spy-1, 0 );
	dspaces_put("mat1", 1, sizeof(char), rank*spx, 0, 0, ((rank+1)*spx)-1, spy-1, 0, matrix);
	dspaces_put("mat2", 1, sizeof(char), rank*spx, 0, 0, ((rank+1)*spx)-1, spy-1, 0, matrix);
	dspaces_put("mat3", 1, sizeof(char), rank*(spx/2), 0, 0, ((rank+1)*(spx/2))-1, spy-1, 0, matrix);

	dspaces_unlock_on_write("lock");

	dspaces_barrier();
	printf("Rank:%d\n", rank);

	if(rank == 0){

//		dspaces_lock_on_read("lock");
		printf("Rank 0, going to get\n\n");
		rank=1;
		char *matrix_ret1 = (char*)malloc(sizeof(char)*spx*spy);
		char *matrix_ret2 = (char*)malloc(sizeof(char)*spx*spy);
		char *matrix_ret3 = (char*)malloc(sizeof(char)*(spx/2)*spy);
		printf("Rank:%d\n", rank);
		//	rank=(rank+1)%size;// proc 1 will retreive proc 0 data, and vice-versa

		printf( "Data Spaces get process rank:%d; total peers: %d <%d,%d,%d;%d,%d,%d> mat1\n", rank, peers,rank*spx, 0, 0, ((rank+1)*spx)-1, spy-1, 0 );
		int err = dspaces_get("mat1", 1, sizeof(char), rank*spx, 0, 0, ((rank+1)*spx)-1, spy-1, 0, matrix_ret1);
		if(err != 0){
			printf("Failed in get operation. Err=%d\n", err);
		}
		err = dspaces_get("mat2", 1, sizeof(char), rank*spx, 0, 0, ((rank+1)*spx)-1, spy-1, 0, matrix_ret2);
		if(err != 0){
			printf("Failed in get operation. Err=%d\n", err);
		}
		err = dspaces_get("mat3", 1, sizeof(char), rank*(spx/2), 0, 0, ((rank+1)*(spx/2))-1, spy-1, 0, matrix_ret3);
		if(err != 0){
			printf("Failed in get operation. Err=%d\n", err);
		}


		printf("Rank:%d\n", rank);
		diff(matrix, matrix_ret1, spx, spy);
		diff(matrix, matrix_ret2, spx, spy);
		diff(matrix, matrix_ret3, spx/3, spy);
		printf("After diff\n");
		free(matrix_ret1);
		free(matrix_ret2);
		free(matrix_ret3);

//		dspaces_unlock_on_read("lock");
	}

	free(matrix);	
	dspaces_barrier();
	dspaces_finalize();
	MPI_Finalize();
	return 0;
}
