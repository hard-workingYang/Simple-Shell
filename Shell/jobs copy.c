#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "jobs.h"
//创建节点
static Node* makeNode(Job *job)
{
    Node * p = (Node *)malloc(sizeof(Node));
    assert(p != NULL);
    p->job = job;
    p->next = NULL;
    return p;
}

static int JInsertTail(JobList *list, Job *job)
{
    Node *ptem = list->head;
    Node *current;
    Node *node;
    node = makeNode(job);
    while(ptem->next){
        ptem = ptem->next;
    }
    ptem->next = node;
    return 1;
}

static int checkPidInJob(Job *job, int pid){
    for(int i = 0; i < job->nums; i++){
        if(job->items[i]->pid == pid)
            return 1;
    }
    return 0;
}

static int setPidInJob(Job *job, int pid, JobState state){
    int isDone = 1;
    for(int i = 0; i < job->nums; i++){
        if(job->items[i]->pid == pid){
            job->items[i]->state = state;
        } 
    }
    return 0;
}

static int checkJobEnd(Job *job){
    int isEnd = 1;
    for(int i = 0; i < job->nums; i++){
        if(job->items[i]->state == Running){
            isEnd = 0;
        } 
    }  
    return isEnd;
}

static void rmNextJobNode(Node *node){
    node->next = node->next->next;
}


//找到返回关键字的节点，否则返回null指针
static Node* JFindPreviosByPid(JobList * list, int pid)
{
    Node* previous = list->head;
    Node* current;
    while ( (current = current->next) != NULL && checkPidInJob(current->job, pid))
        previous = previous->next;
    if ( current == NULL )
        return 0;
    return previous;
}

//带头节点的单链表
static void JInit(JobList * list)
{
    list->head = makeNode(NULL);
}

static int JModifyState(JobList* list, int pid, JobState state){
    Node *previous = JFindPreviosByPid(list, pid);
    if(previous->next == NULL)
        return 0;
    setPidInJob(previous->next->job, pid, state);
    //如果对应的都删除了，就直接移除对应的job
    if(checkJobEnd(previous->next->job)){
        Node *current = previous->next;
        rmNextJobNode(previous);
        free(current);
    }
    return 1;
}


//遍历
void JTreaverse(JobList * JobList, void (*func) (Node* p) )
{
    Node * current = JobList->head;
    int pos = 0;
    func(current);
    while ( (current = current->next) != NULL )
        func(current);
}

static void treverseJob(Node* p, int pos){
    Job *job = p->job;
    for(int i = 0 ;i < job->nums; i++){
        if(i == 0)
            printf("[%d] ",  pos);
        else
            printf("     ");
        JobItem *item = job->items[i];
        printf("%d", item->pid);
        switch (item->state)
        {
            case Running:
                printf("%9s", "Running");
                break;
            case Stopped:
                printf("%9s", "Stopped");
                break;
            case Done:
                printf("%9s", "Done");
                break;    
            default:
                break;
        }   
        printf("%s\n", item->paras);
    }
}
int JTreaverseJobs(JobList *jobList)
{
    Node * current = jobList->head;
    int pos = 0;
    char c;
    return 1;
    // treverseJob(current, pos++);
    while((current = current->next) != NULL)
        treverseJob(current, pos++);
    return 1;
}


//销毁节点
static void destroyNode(Node* node)
{
    free((Node *) node);
}

//销毁节点
void JDestrory(JobList * list)
{
    JTreaverse(list, destroyNode);
    free(list->head);
}


//修改,先删后插入，因为这是有序链表
// int LModify(JobList * list, const int key, const int data)
// {
//     if( LRemove(list, key) )
//         LInsert(list, data);
//     else
//         return 0;
//     return 1;
// }

//找到返回关键字的节点，否则返回null指针
// Node* LFind(JobList * list, const int key)
// {
//     Node * current = list->head;
//     while ( (current = current->next) != NULL && current->data != key)
//         if( current->data > key )
//             return NULL;
//     return current;
// }

void init_joblist(){
    g_JobList = (JobList *)malloc(sizeof(JobList));
	JInit(g_JobList);
    g_JobNum = 0;
}
int show_jobs(){
	JTreaverseJobs(g_JobList);
	return 1;
}


int add_jobs(Job *job){
    return JInsertTail(g_JobList, job);
}

static Job* new_jobs(){
    Job *job = (Job*)malloc(sizeof(Job));
    return job;
}

// int add_jobitem(Job *job, JobItem *item){
//     if(job->nums >= JOB_ITEM_MAX)
//         return 0;
//     job->items[job->nums++] = item;
//     return 1;
// }
int add_jobitem(Job *job, char *name, char *paras, int pid){
    if(job->nums >= JOB_ITEM_MAX)
        return 0;
    JobItem* item = (JobItem*)malloc(sizeof(JobItem));
    strcpy(item->name, name);
    strcpy(item->paras, paras);
    item->pid = pid;
    item->state = Running;
    job->items[job->nums++] = item;
    return 1;
}

Job* init_jobs(char *name, char *paras, int pid){
    Job *job = new_jobs();
    add_jobitem(job, name, paras, pid);
}

int modify_pidstate(int pid, JobState state){
    return JModifyState(g_JobList, pid, state);
}
