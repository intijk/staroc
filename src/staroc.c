#include <mpi.h>
#include <stdio.h>
#include "include/dataType.h"
int main(int argc, char* argv[])
{
	int rank,size;
	NodeInfo ni;
	MPI_Status status;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	ni.status=1;
	if(rank==0){
	//	printf("Rank %d\n",rank);

		doMaster(argc,argv);
	}else{	
		//printf("Rank %d\n",rank);
		doSlave();
	}
	MPI_Finalize();
	return 0;
}

