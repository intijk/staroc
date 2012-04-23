#ifndef DEFINITION_H
#define DEFINITION_H


void doMaster(int argc, char* argv[]);
void doSlave(int rank);
#define true 1
#define false 0
#define RET_SUCCESS 0
#define RET_FAILED -1

#define MAX_LENGTH 1024
#define MAX_TASK_ONCE 1024

#define APPSTART 0
#define TASK_SEND 1
#define TASK_RET 2
typedef struct TaskDeployInfo{
		int timeLimit[MAX_TASK_ONCE]; //单位是秒
		int n;
		int list[MAX_TASK_ONCE];
		time_t sendTime;//发送时间戳
}TaskDeployInfo;

#define TASK_RET_NOSTARTED 0
#define TASK_RET_TIMEOUT 1
#define TASK_RET_RUNERROR 2
#define TASK_RET_SUCCESS 3

#define TASK_NODE_NOTSTARTED 0
#define TASK_NODE_STARTED 1
#define TASK_NODE_FINISH 2
typedef struct  TaskReturnInfo{
		time_t finishTime[MAX_TASK_ONCE];
		int listStatus[MAX_TASK_ONCE];
}TaskReturnInfo;

#define TASK_WAITING 0
#define TASK_PREPARE 1
#define TASK_COMPUTING 2
#define TASK_SUCCESS 3
typedef struct TaskInfo{
		char status;
		char failedReason;
		time_t timeLimit;
		int nodeNumber;
		time_t startTime;
		time_t endTime;
}TaskInfo;					

#define NODE_IDLE 0
#define NODE_SENDING 1
#define NODE_COMPUTING 2
#define NODE_RECEIVING 3
#define NODE_EXITING 4
#define NODE_EXITED 5
#define NODE_FINISH 6
typedef struct NodeInfo{
		char status;
		int completed;
		int failed;
		double TotalSucTime;
		double TotalFailTime;
		struct TaskDeployInfo D;
		struct  TaskReturnInfo R;
		MPI_Request mpi_request;
}NodeInfo;

#endif
