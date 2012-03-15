#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include "include/definition.h"


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
	strcat(appInitName, appName);
	strcat(appInitName, "_init");
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
				  fprintf(fpLog,"to node %d\n", nodeNum);
				  
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
				fprintf(fpLog,"Node %d finish task:Total(%d),SUCCESS(%d),FAILED(%d)\n", nodeNum, sucTaskNum, failTaskNum);
				if(sucTaskNum>0){
						fprintf(fpLog,"Success:");
						for(i=0;i<sucTaskNum;i++){
							fprintf(fpLog,"%d ",sucTask[i]);
						}
						fprintf(fpLog,"\n");
				}
				if(failTaskNum>0){
						fprintf(fpLog,"Failed:");
						for(i=0;i<failTaskNum;i++){
								fprintf(fpLog,"%d ",failTask[i]);
						}
						fprintf(fpLog,"\n");
				}
				CompleteTaskNum+=sucTaskNum;
				break;
			}
	}
}
void config_init(){
	char appConfigResultName[MAX_LENGTH]={0};
	strcat(appConfigResultName, appName);	
	strcat(appConfigResultName, "_config_result");
	FILE *fpConfig=fopen(appConfigResultName,"r");
	fscanf(fpConfig,"%d",&TotalTaskNum);
	fscanf(fpConfig,"%d",&TimeLimitPerTask);
	fscanf(fpConfig,"%d",&TotalTimeLimit);


	//为简单起见，认为每个任务的时限是相同的，以后会修改	
	int i;
	for(i=1;i<=TotalTaskNum;i++){
			TaskL[i].timeLimit=TimeLimitPerTask;
	}
	fpLog=fopen("master.log","w");
	update_master_info(APPSTART,0);	
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
	if(taskNum==0)return 0;//no task is waiting,they are working on or done
	NodeL[nodeNum].D.timeLimit=TaskL[taskNum].timeLimit;
	NodeL[nodeNum].D.n=1;
	NodeL[nodeNum].D.list[0]=taskNum;
	time_t t;
	NodeL[nodeNum].D.sendTime=time(&t);

	int ret=MPI_ISend(&NodeL[nodeNum].D, sizeof(TaskDeployInfo), MPI_BYTE, nodeNum, 0, MPI_COMM_WORLD, &NodeL[nodeNum].mpi_request);

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
						   int taskNum;
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
		for(i=1;i<=TotalHostNum;i++){
				MPI_ISend();
		}
		int suc=0;
		while(1){
				for(i=1;i<=TotalHostNum;i++){
						MPI_Test();
						if(success){
								suc++;
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
		fprintf(fpLog, "STAROC task %s ended at %s\n",appName,ctime(&t));	
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
					fprintf("%d ", i);
			}
		}
		fprintf(fpLog, "\n");
		fprintf(fpLog, "Failed Task List:\n");
		for(i=1;i<=TotalTaskNum;i++){
				if(Task[i].status!=TASK_SUCCESS){
						fprintf("%d ", i);
				}
		}
		fprintf(fpLog, "\n");
}
void doMaster(int argc, char* argv[]){
	appName=argv[1];//Client Application Name
	sscanf(argv[2],"%d",&TotalHostNum);
	client_init();//客户初始化程序:1
	config_init();//系统初始化信息:2

	NodeL=(NodeInfo*)malloc(sizeof(NodeInfo) * (TotalHostNum+1));
	memset(NodeL,sizeof(NodeInfo) * (TotalHostNum+1), 0);
	TaskL=(TaskInfo*)malloc(sizeof(TaskInfo) * (TotalTaskNum+1));
	memset(TaskL,sizeof(TaskInfo) * (TotalTaskNum+1), 0);
			

	time_t startTime,currentTime;	
	time(&startTime);
	while(CompleteTaskNum<TotalTaskNum){
		schedule(TaskL,NodeL);//优先级调度算法：3
		time(&currentTime);
		if(currentTime-startTime >=TotalTimeLimit){
				finishAll();
				break;
		}
	}
	finishMaster();
	upload_master_record();//上传记录:7
	end();//结束计算：8
}
