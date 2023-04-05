#include<sys/types.h>

#ifndef __JOBS_H__
#define __JOBS_H__

#define MAXJOBS 32
#define MAXJOBITEMS 10
#define MAXARGS 256
#define MAXJID  1<<16 //这个用别人的，处理很妙
#define MAXCMDLINE 1024

#define S_RUNNING 0
#define S_SUSPENDED 1
#define S_DONE 2
#define S_KILLED 3
#define S_ABORTED 4

typedef struct {
	int state;
	char cmd[MAXCMDLINE];
	pid_t pid;
} JobItem;

typedef struct{
	JobItem items[MAXJOBITEMS];
	int nums;
    int inuse;
} Job;

Job g_Jobs[MAXJOBS];

int g_MinPossIdx;      //可能可用的最小idx
int g_MaxInuseIdx;     //已经在用的最大idx
int g_LatestIdx;       //最新创建job的idx
int g_SecondLatestIdx; //次新job的idx
int g_FgJid;           //当前的前台作业jid(前台作业一次应该只有一个)

//大调数据结构，因为发现jobs的结构应该是数组结构
int check_jidAvail(int jid);
int init_jobs();
int add_job(pid_t pid, int state, char *cmd);
int add_p2job(int jid, pid_t pid, int state, char *cmd);
int pid2jid(pid_t pid);
int list_jobs();
int chg_state_bypid(pid_t pid, int state);
int chg_state_byjid(int jid, int state);
int check_jobend(int jid);
int rm_byjid(int jid);
int process_job(int jid, void (*func)(JobItem *));

int get_fgjid();
int get_latestJobid();
int get_secondLatestJobid();

void set_fgjid(int jid);
int clear_fgjid(int jid);

#endif 
