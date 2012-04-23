#include <mpi.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "include/definition.h"
int main(int argc, char* argv[])
{
	int rank,size;
	printf("Initialize mpi...\n");
	fflush(stdout);
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	if(rank==0){
	//	printf("Rank %d\n",rank);

		doMaster(argc,argv);
	}else{	
		//printf("Rank %d\n",rank);
		doSlave(rank);
	}
	MPI_Finalize();
	return 0;
}

