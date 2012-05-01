#include <mpi.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include "include/definition.h"

FILE* fpLog;
int rank;
TaskDeployInfo D,De;
TaskReturnInfo R;
pid_t PidL[MAX_TASK_ONCE];
MPI_Request mpi_request,mpi_request_exit;
int status;
int CompleteTaskNum;
int FailedTaskNum;
char appName[MAX_LENGTH];
char appComputeName[MAX_LENGTH];

void slave_init(){
	char logName[MAX_LENGTH]={0};
	time_t currentTime;
	time(&currentTime);
	sprintf(logName, "node%d.log",rank);
	fpLog=fopen(logName, "w");
	fprintf(fpLog, "Node %d started at %s\n", rank, ctime(&currentTime));
	MPI_Irecv(&De, sizeof(D), MPI_BYTE, 0, 1, MPI_COMM_WORLD, &mpi_request_exit);
}
pid_t startTask(int taskNum){
	pid_t ret=fork();
	char taskNumString[MAX_LENGTH];
	sprintf(taskNumString, "%d", D.list[taskNum]);
	if(ret<0){
		//error###
	}
	if(ret>0){
		//parent
		return ret;
	}
	if(ret==0){
		execl(appComputeName, appComputeName, taskNumString, NULL);
	}
	return -1;
}
int compute_rolling(){
	static int i=0;
	static pid_t pid;
	static time_t startTime,finishTime;
	static int statloc;
	static int timeOut=false;
	switch(R.listStatus[i]){
		case TASK_NODE_NOTSTARTED:{
						  time(&startTime);
						  timeOut=false;
						  fprintf(fpLog, "Start task %d at %s",D.list[i],ctime(&startTime)); 
						  pid=startTask(i);
						  R.listStatus[i]=TASK_NODE_STARTED;
						  break;
					  }
		case TASK_NODE_STARTED:{
					       time(&finishTime);
					       if(finishTime - startTime >= D.timeLimit[i]){
						       kill(pid,SIGKILL);
						  timeOut=true;
						  fprintf(fpLog, "Start task %d at %s",D.list[i],ctime(&startTime)); 
								
					       }
					       int ret=waitpid(pid,&statloc,WNOHANG);
					       if( ret!=0 && ( WIFEXITED(statloc) || WIFSIGNALED(statloc)) ){
						       //printf("Task %d finished \n", i);

						       R.listStatus[i]=TASK_NODE_FINISH; 
					       }
					       break;
				       }
		case TASK_NODE_FINISH:{
					      fprintf(fpLog, "Finish task %d , result: ", D.list[i]); 
					      if(!(WIFSIGNALED(statloc)) && WEXITSTATUS(statloc)==RET_SUCCESS){
						      fprintf(fpLog, "SUCCESS");
						      R.listStatus[i]=TASK_RET_SUCCESS;
					      }else if(timeOut){
						      fprintf(fpLog,"TIMEOUT");
					      }else{
						      R.listStatus[i]=TASK_RET_RUNERROR;
						      fprintf(fpLog,"RUNERROR");
					      }
					      fprintf(fpLog," at %s",ctime(&finishTime));
					      i++;
					      break;
				      }

	}
	fflush(fpLog);
	if(i==D.n){
		i=0;
		return 0;
	}else{
		return D.n-i;
	}
}
void task_rolling(){
	MPI_Status mpi_status;
	int flag;
	while(1){
		MPI_Test(&mpi_request_exit, &flag, &mpi_status);	//exit deploy;
		if(flag==true){
			status=NODE_EXITING;
		}
		switch(status){
			case NODE_IDLE:{
					       MPI_Irecv(&D, sizeof(D), MPI_BYTE, 0, 0, MPI_COMM_WORLD, &mpi_request);
					       status=NODE_SENDING;	
					       break;
				       }
			case NODE_SENDING:{
						  MPI_Test(&mpi_request,&flag,&mpi_status);
						  if(flag==true){
							  fprintf(fpLog,"Received Task:");
							  int i;
							  for(i=0;i<D.n;i++){
								  R.listStatus[i]=TASK_NODE_NOTSTARTED;
								  fprintf(fpLog," %d",D.list[i]);
							  }
							  time_t t;
							  time(&t);
							  fprintf(fpLog," at %s", ctime(&t));
							  status=NODE_COMPUTING;
						  }else{
							  //just wait
						  }
						  break;
					  }
			case NODE_COMPUTING:{
						    if(compute_rolling()==0){
							    status=NODE_FINISH;
						    }
						    break;
					    }
			case NODE_FINISH:{
						 MPI_Isend(&R,sizeof(R),MPI_BYTE,0,0,MPI_COMM_WORLD,&mpi_request);
						 status=NODE_RECEIVING;
						 break;
					 }
			case NODE_RECEIVING:{
						    MPI_Test(&mpi_request,&flag,&mpi_status);
						    if(flag==true){
							    fprintf(fpLog,"Return finish , now being idle.\n\n");
							    status=NODE_IDLE;
						    }
						    break;
					    }
			case NODE_EXITING:{
						  //killall();
						  //upload_log(); ###
						  fprintf(fpLog,"Receive exit notification , exiting ... \n");
						  return ;
					  }
		}
	}
}

void receive_app_info(){
	//Receiv app name 
	MPI_Status mpi_status;
	int appNameLength;
	MPI_Recv(&appNameLength, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &mpi_status);
	MPI_Recv(appName, appNameLength, MPI_BYTE, 0,0, MPI_COMM_WORLD, &mpi_status);
	appName[appNameLength]='\0';
	sprintf(appComputeName, "./%s_compute",appName);
}
void doSlave(int me){

	rank=me;	
	receive_app_info();
	slave_init();//开节点日志：1
	task_rolling();
	//	wait_task();//等任务:2
	//	run_client();//运行客户程序:3
	//	update_slave_record();//更新记录状态:4
	//	send_back_result();//返回结果:5
}
