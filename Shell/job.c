#include <stdio.h>
#include <string.h>
#include "job.h"
#include "signal.h"

Job g_Jobs[MAXJOBS];

int g_MinPossIdx;      //可能可用的最小idx
int g_MaxInuseIdx;     //已经在用的最大idx
int g_LatestIdx;       //最新创建job的idx
int g_SecondLatestIdx; //次新job的idx
int g_FgJid;           //当前的前台作业jid(前台作业一次应该只有一个)

int check_jidAvail(int jid){
    if(jid >= MAXJOBS || g_Jobs[jid].inuse == 0)
        return 0;
    return 1;
}
int init_jobs(){
    for(int i = 0; i < MAXJOBS; i++){
        g_Jobs[i].inuse = 0;
    }
    g_MinPossIdx = 0;
    g_MaxInuseIdx = 0;
    g_LatestIdx = -1;
    g_SecondLatestIdx = -1;
    g_FgJid = -1;
    return 1;
}

//新的作业
int add_job(pid_t pid, int state, char *cmd){
    if(pid < 0){
        return 0;
    }

    //找到一个新的空闲位置 从最小的可能位置出发
    int i = g_MinPossIdx;
    while(i < MAXJOBS){
        if(g_Jobs[i].inuse == 0){
            g_Jobs[i].inuse = 1;
            g_Jobs[i].nums = 1;
            g_Jobs[i].items[0].state = state;
            g_Jobs[i].items[0].pid = pid;
			strcpy(g_Jobs[i].items[0].cmd, cmd);
            g_MinPossIdx = i + 1;
            break;
        }
        i++;
    }
    if(i >= MAXJOBS)
        return -1;

    if(i > g_MaxInuseIdx)
        g_MaxInuseIdx = i;

    if(g_LatestIdx != -1)
        g_SecondLatestIdx = g_LatestIdx;
    g_LatestIdx = i;
    return i;
}

//新的pid 在原来的job中加入新的进程
int add_p2job(int jid, pid_t pid, int state, char *cmd){

    if(!check_jidAvail(jid)){
        return 0;
    }
    
    //加入新的进程
    if(g_Jobs[jid].nums >= MAXJOBITEMS){
        return 0;
    }
    
    int idx = g_Jobs[jid].nums++;
    g_Jobs[jid].items[idx].state = state;
    g_Jobs[jid].items[idx].pid = pid;
    strcpy(g_Jobs[jid].items[idx].cmd, cmd);

    return 1;
}

int pid2jid(pid_t pid){

    for(int i = 0; i <= g_MaxInuseIdx; i++){
        if(g_Jobs[i].inuse == 0)
            continue;

        for(int j = 0; j < g_Jobs[i].nums; j++){
            if(g_Jobs[i].items[j].pid == pid){
                return i;
            }
        }
    }

    return -1;
}

int list_jobs(){

    for(int i = 0; i <= g_MaxInuseIdx; i++){
        if(g_Jobs[i].inuse == 0)
            continue;

        char tag = ' ';
        if(g_LatestIdx == i)
            tag = '+';
        else if(g_SecondLatestIdx == i)
            tag = '-';
        
        printf("[%d]  %c ", i, tag);
        for(int j = 0; j < g_Jobs[i].nums; j++){
            if(j != 0){
                printf("%7s", "");
            }
            printf("%6d ", g_Jobs[i].items[j].pid);
            switch (g_Jobs[i].items[j].state)
            {
            case S_RUNNING:
                printf("%10s ", "running");
                break;
            case S_SUSPENDED:
                printf("%10s ", "suspended");
                break;
            case S_DONE:
                printf("%10s ", "done");
                break;
            case S_KILLED:
                printf("%10s ", "killed");
            case S_ABORTED:
                printf("%10s ", "aborted");
                break;
            default:
                break;
            }
            printf("%s ",g_Jobs[i].items[j].cmd);
            printf("\n");
        }
    }
    return 1;
}

int chg_state_bypid(pid_t pid, int state){

    for(int i = 0; i <= g_MaxInuseIdx; i++){
        if(g_Jobs[i].inuse == 0)
            continue;

        for(int j = 0; j < g_Jobs[i].nums; j++){
            if(g_Jobs[i].items[j].pid == pid){
                //找到pid了
                g_Jobs[i].items[j].state = state;

                //如果是停止的信号修改，那么判断一下对应的整个job是否都结束了，是就移除job
                if(state == S_DONE || state == S_ABORTED || state == S_KILLED){
                    if(check_jobend(i)){
                        rm_byjid(i);
                    }
                }  

                return i;
            }
        }
    }

    return -1;
}

int chg_state_byjid(int jid, int state){
    for(int i = 0; i < g_Jobs[jid].nums; i++){
        g_Jobs[jid].items[i].state = state;
    }
    return 1;
}

int check_jobend(int jid){

    //判断job是否存在
    if(!check_jidAvail(jid))
        return 0;

    for(int i = 0; i < g_Jobs[jid].nums; i++){
        if(g_Jobs[jid].items[i].state == S_RUNNING || g_Jobs[jid].items[i].state == S_SUSPENDED){
            return 0;
        }
    }
    //到这里说明结束了
    return 1;
}

//移除
int rm_byjid(int jid){
    //
    if(!check_jidAvail(jid))
        return 0;
    g_Jobs[jid].inuse = 0;
    if(jid < g_MinPossIdx)
        g_MinPossIdx = jid;
    if(g_MaxInuseIdx == jid)
        g_MaxInuseIdx--;
    if(g_LatestIdx == jid)
        g_LatestIdx = -1;
    if(g_SecondLatestIdx == jid)
        g_SecondLatestIdx = -1;
    if(g_FgJid == jid)
        g_FgJid = -1;
    return 1;
}

int process_job(int jid, void (*func)(JobItem *)){

    if(!check_jidAvail(jid))
        return 0;
    
    for(int i = 0; i < g_Jobs[jid].nums; i++){
        func(&g_Jobs[jid].items[i]);
    }
    return 1;
}


int get_fgjid(){
    return g_FgJid;
}

int get_latestJobid(){
    return g_LatestIdx;
}
int get_secondLatestJobid(){
    return g_SecondLatestIdx;
}
void set_fgjid(int jid){
    g_FgJid = jid;
}

int clear_fgjid(int jid){
    if(g_FgJid == jid){
        g_FgJid = -1;
        return 1;
    }
    return 0;
}