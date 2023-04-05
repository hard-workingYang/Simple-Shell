#ifndef __JOBS_H__
#define __JOBS_H__

#define MAX_JOB_NUM 100
#define JOB_ITEM_MAX 10

typedef enum{
	Running = 0,
	Stopped,
	Done,
}JobState;

typedef struct {
	JobState state; //running/stopped
	char name[100];
	char paras[200];
	int pid;
} JobItem;
typedef struct{
	JobItem* items[JOB_ITEM_MAX];
	int nums;
} Job;
typedef struct node
{
    Job *job;
    struct node * next;
} Node;


//链表
typedef struct linklist
{
    Node * head;
} JobList;

//两个全局变量，一个是链表list，另一个是列表pid
int g_JobNum;
JobList *g_JobList;

// int JRemove(JobList * list, int pid);

// int JStopPid(JobList * list, int pid);
// int JTreaverseJobs(JobList *JobList);

//一个是对job的处理
void init_joblist();
Job* init_jobs(char *name, char *paras, int pid);
int add_jobs(Job *job);
int show_jobs();
int modify_pidstate(int pid, JobState state);

//bg和fg指令的实现接口


#endif 
