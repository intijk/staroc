#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include "include/definition.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>



char* appName;//Client Application Name
int TotalHostNum; //Total cpu num, where in 'mpirun -np TotalHostsNum'
int TotalTaskNum;
int TimeLimitPerTask;
int TotalTimeLimit;
int CompleteTaskNum;
NodeInfo* NodeL;
TaskInfo* TaskL;
FILE *fpLog;

void client_init(){
	char appInitName[MAX_LENGTH]={0};
	strcat(appInitName, "./");
	strcat(appInitName, appName);
	strcat(appInitName, "_init");
	//文件不存在,表示用户只想执行子程序, 没有初始化程序
	if((access(appInitName, F_OK) !=0)){
		printf("%s not exit, skip user init.\n", appInitName);
		return ;
	}
	fflush(stdout);
	int ret=system(appInitName);
	if(!(WIFEXITED(ret) && WEXITSTATUS(ret) == RET_SUCCESS)){
		fprintf(stderr, "Client init %s run error \n", appInitName);
		exit(-1);
	}
}
void update_master_info(int operation, int par1){
		time_t currentTime;
		time(&currentTime);
		switch(operation){
			case APPSTART:{
				  fprintf(fpLog,"STAROC task %s started at %s\n",appName,ctime(&currentTime));
				  fflush(fpLog);
				  break;
			}
			case TASK_SEND:{
				  int nodeNum=(int)par1;
				  int i;
				  fprintf(fpLog,"Send task ");
				  for(i=0;i<NodeL[nodeNum].D.n;i++){
						  int taskNum=NodeL[nodeNum].D.list[i];
						  fprintf(fpLog,"%d ", taskNum);
						  TaskL[taskNum].status=TASK_PREPARE;
						  TaskL[taskNum].startTime=NodeL[nodeNum].D.sendTime;
						  TaskL[taskNum].nodeNumber=nodeNum;
				  }
				  fprintf(fpLog,"to node %d at %s", nodeNum, ctime(&currentTime));
				  fflush(fpLog); 
				  break;
			}
			case TASK_RET:{
			    int i;
				int nodeNum=(int)par1;
				int sucTaskNum=0;
				int failTaskNum=0;
				int sucTask[MAX_TASK_ONCE]={0};
				int failTask[MAX_TASK_ONCE]={0};

				for(i=0;i<NodeL[nodeNum].D.n;i++){
					//updte node info
					int taskNum=NodeL[nodeNum].D.list[i];
					if(NodeL[nodeNum].R.listStatus[i]==TASK_RET_SUCCESS){
							sucTask[sucTaskNum++]=taskNum;
							NodeL[nodeNum].completed++;
							NodeL[nodeNum].TotalSucTime+=NodeL[nodeNum].R.finishTime[i]-TaskL[taskNum].startTime;
					}else{
							failTask[failTaskNum++]=taskNum;
							NodeL[nodeNum].failed++;
							NodeL[nodeNum].TotalFailTime+=NodeL[nodeNum].R.finishTime[i]-TaskL[taskNum].startTime;
					}
					//updat task info
					if(NodeL[nodeNum].R.listStatus[i]!=TASK_RET_SUCCESS){
							TaskL[taskNum].failedReason=NodeL[nodeNum].R.listStatus[i];
							TaskL[taskNum].status=TASK_WAITING;
					}else{
							TaskL[taskNum].status=TASK_SUCCESS;
					}
					TaskL[taskNum].endTime=NodeL[nodeNum].R.finishTime[i];
				}
				fprintf(fpLog,"Node %d finish task:Total(%d),SUCCESS(%d),FAILED(%d). ", nodeNum, NodeL[nodeNum].D.n, sucTaskNum, failTaskNum);
				if(sucTaskNum>0){
						fprintf(fpLog,"Success:");
						for(i=0;i<sucTaskNum;i++){
							fprintf(fpLog," %d",sucTask[i]);
						}
						fprintf(fpLog,";");
				}
				if(failTaskNum>0){
						fprintf(fpLog,"Failed:");
						for(i=0;i<failTaskNum;i++){
								fprintf(fpLog," %d",failTask[i]);
						}
				}
				fprintf(fpLog,"\n");
				CompleteTaskNum+=sucTaskNum;
				break;
			}
	}
}

void config_init(int argc, char* argv[]){
	char appConfigResultName[MAX_LENGTH]={0};

	fpLog=fopen("master.log","w");
	if(fpLog==NULL){
			fprintf(fpLog, "Open master.log for write failed.\n");
			exit(-1);
	}
	strcat(appConfigResultName, appName);	
	strcat(appConfigResultName, "_config_result");
	FILE *fpConfig=fopen(appConfigResultName,"r");

	if(fpConfig==NULL){
			fprintf(fpLog, "Open %s failed.\n", appConfigResultName);
			printf("%s not exit, take default config.\n",appConfigResultName);
			sscanf(argv[2], "%d", &TotalTaskNum);
			TotalTaskNum--;
			TimeLimitPerTask=5;
			TotalTimeLimit=10;
	}else{
		fscanf(fpConfig,"%d",&TotalTaskNum);
		fscanf(fpConfig,"%d",&TimeLimitPerTask);
		fscanf(fpConfig,"%d",&TotalTimeLimit);
	}	
	NodeL=(NodeInfo*)malloc(sizeof(NodeInfo) * (TotalHostNum+1));
	memset(NodeL,0, sizeof(NodeInfo) * (TotalHostNum+1));
	TaskL=(TaskInfo*)malloc(sizeof(TaskInfo) * (TotalTaskNum+1));
	memset(TaskL,0, sizeof(TaskInfo) * (TotalTaskNum+1));


	//为简单起见，认为每个任务的时限是相同的，以后会修改	
	int i;
	for(i=1;i<=TotalTaskNum;i++){
	 		TaskL[i].timeLimit=TimeLimitPerTask;
	}

	update_master_info(APPSTART,0);	


	int appNameLength=strlen(appName);
	for(i=1;i<=TotalHostNum;i++){
	MPI_Send(&appNameLength, 1 , MPI_INT, i, 0, MPI_COMM_WORLD);
	MPI_Send(appName, appNameLength, MPI_BYTE, i, 0, MPI_COMM_WORLD);
	}


}
int getATask(){
		int i;
		for(i=1;i<=TotalTaskNum;i++){
				if(TaskL[i].status==TASK_WAITING){
					return i;
				}
		}
		return 0;
}
int send_task(int nodeNum){
	//当前最简单的单一调度算法
	int taskNum=getATask();	
	if(taskNum==0){
			//fprintf(fpLog,"no task is waiting,they are working on or done");
			return RET_FAILED;//no task is waiting,they are working on or done
	}
	NodeL[nodeNum].D.timeLimit[0]=TaskL[taskNum].timeLimit;
	NodeL[nodeNum].D.n=1;
	NodeL[nodeNum].D.list[0]=taskNum;
	time_t t;
	NodeL[nodeNum].D.sendTime=time(&t);

	int ret=MPI_Isend(&NodeL[nodeNum].D, sizeof(TaskDeployInfo), MPI_BYTE, nodeNum, 0, MPI_COMM_WORLD, &NodeL[nodeNum].mpi_request);

	if(ret!=MPI_SUCCESS){
		return RET_FAILED;
	}

	return RET_SUCCESS;	
}
void schedule(){
		int flag,i;
		MPI_Status mpi_status;
		for(i=1;i<=TotalHostNum;i++){
			switch(NodeL[i].status){
			case NODE_IDLE: {
						   if(send_task(i)==RET_SUCCESS){
								   NodeL[i].status=NODE_SENDING;
								   update_master_info(TASK_SEND,i);
						   }else{
								   //###任务发送失败
						   }
						   break;
						   
					   }
			case NODE_SENDING: {
								  MPI_Test(&NodeL[i].mpi_request,&flag,&mpi_status);
								  if(flag==true){
										  NodeL[i].status=NODE_COMPUTING;
								  }else{
										  //没有发送成功,等待即可
								  }
								  break; 
						  }
			case NODE_COMPUTING: {
								   MPI_Irecv(&NodeL[i].R, sizeof(TaskReturnInfo), MPI_BYTE, i, 0, MPI_COMM_WORLD, &NodeL[i].mpi_request);
								   NodeL[i].status=NODE_RECEIVING;
								   break;
						   }
			case NODE_RECEIVING:{
								   MPI_Test(&NodeL[i].mpi_request,&flag,&mpi_status);
								   if(flag==true){
										   NodeL[i].status=NODE_IDLE;
										   //检查任务是否完成
										   update_master_info(TASK_RET,i);
								   }else{
										   //没有接收完成，等待即可
								   }
						   }
			}
		}
}
void finishAll(){
		int i;
		time_t t;
		int ret;
		//发送结束包，通知结束
		for(i=1;i<=TotalHostNum;i++){
				NodeL[i].D.n=0;
				NodeL[i].D.sendTime=time(&t);
				ret=MPI_Isend(&NodeL[i].D,sizeof(NodeL[i].D),MPI_BYTE,i,1,MPI_COMM_WORLD, &NodeL[i].mpi_request );
				if(ret!=MPI_SUCCESS){
						fprintf(fpLog,"Send finish Error\n");
						fprintf(stderr,"Send finish Error\n");
				}
				NodeL[i].status=NODE_EXITING;
		}
		int suc=0;
		//轮询是否发送完毕
		while(1){
				int flag;
				MPI_Status mpi_status;
				for(i=1;i<=TotalHostNum;i++){
						if(NodeL[i].status==NODE_EXITING){
								MPI_Test(&NodeL[i].mpi_request,&flag,&mpi_status);
								if(flag==true){
										NodeL[i].status=NODE_EXITED;
										suc++;
								}
						}
				}
				if(suc==TotalHostNum){
		
						return ;
				}
		}
}
void finishMaster(){
		
		time_t t;
		time(&t);
		fprintf(fpLog, "\n");
		fprintf(fpLog, "STAROC task %s ended at %s",appName,ctime(&t));	
		fprintf(fpLog, "Reason: " );
		if(CompleteTaskNum==TotalTaskNum){
				fprintf(fpLog,"Success\n");
		}else{
				fprintf(fpLog,"Timeout\n");
		}

		
		fprintf(fpLog, "Complete Task List:\n");
		int i;
		for(i=1;i<=TotalTaskNum;i++){
			if(TaskL[i].status==TASK_SUCCESS){
					fprintf(fpLog, "%d ", i);
			}
		}
		fprintf(fpLog, "\n");
		fprintf(fpLog, "Failed Task List:\n");
		for(i=1;i<=TotalTaskNum;i++){
				if(TaskL[i].status!=TASK_SUCCESS){
						fprintf(fpLog, "%d ", i);
				}
		}
		fprintf(fpLog, "\n");
		fflush(fpLog);
}

void doMaster(int argc, char* argv[]){
	appName=argv[1];//Client Application Name
	sscanf(argv[2],"%d",&TotalHostNum);

	TotalHostNum--;
	printf("Run client init...\n");
	fflush(stdout);
	client_init();//客户初始化程序:1

	printf("Start master...\n");
	fflush(stdout);
	config_init(argc,argv);//系统初始化信息:2
	time_t startTime,currentTime;	
	time(&startTime);
	while(CompleteTaskNum<TotalTaskNum){
		schedule(TaskL,NodeL);//优先级调度算法：3
		time(&currentTime);
		if(currentTime-startTime >=TotalTimeLimit){
			break;
		}
	}

	finishAll();
	finishMaster();

	//upload_master_record();//上传记录:7
	//end();//结束计算：8
}
