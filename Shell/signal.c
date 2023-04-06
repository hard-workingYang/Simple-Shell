#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include "signal.h"
#include "job.h"
#include <errno.h>

//处理子进程
void sigchld_handler(int signum)
{
    printf("sigchld_handler\n");
    //等待回收子进程 然后看一下子进程为什么结束
    int status;
    int pid;
    //下面处理过程中增加了一个阻塞处理（应该是阻塞，即暂时不处理，但后面恢复后信号仍然存在）
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);

    // while((pid = waitpid(-1, &status, 0)) > 0) {
    //增加了WUNTRACED用于直接接受WIFSTOPPED信息
    //WNOHANG用于直接返回避免阻塞（毕竟这里用while是因为可能有多次信号但这里只有一次显示，所以用while处理多个）
    while((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0) {
        //阻塞信号保护一下执行内容
		sigprocmask(SIG_BLOCK, &mask, NULL);
        if(WIFEXITED(status)){
            chg_state_bypid(pid, S_DONE); //正常结束
        }
        else if(WIFSIGNALED(status)){ //因为某种信号导致中断
            chg_state_bypid(pid, S_ABORTED);
        } else if(WIFSTOPPED(status)){ //暂停
            chg_state_bypid(pid, S_SUSPENDED);
        }
		sigprocmask(SIG_UNBLOCK, &mask, NULL);
	}
    return;
}

static void sigtstp_pid(JobItem *jobitem){
    if(jobitem->state == S_RUNNING){
    //    if(kill(-jobitem->pid, SIGSTOP) == 0){
        int ret;
        if((ret = kill(jobitem->pid, SIGTSTP)) == 0){
            // printf("modify %d to suspended\n", jobitem->pid);
            jobitem->state = S_SUSPENDED;
        }
        // printf("ret: %d, errno = %d\n", ret, errno);
    }
}

//ctrl z
void sigtstp_handler(int signum){
    printf("sigtstp_handler\n");

    MaskAll();

    int jid = get_fgjid();
    if(jid < 0){
        UnMaskAll();
		return; //此时没有前台作业
    }
    
    process_job(jid, sigtstp_pid);

    //将job中可以暂停的进程都暂停
    //原来是遍历每一个线程后发送信号，此时kill函数中的值应该为正数（因为组id修改了，使用负数可能会报错）
    //现在改为直接向组id发送kill
    // int job_pgid = get_jobpgid(jid);
    // int ret = kill(-job_pgid, SIGTSTP);
    // printf("job_pgid: %d\n", job_pgid);
    // printf("ret: %d, errno = %d\n", ret, errno);

    //清除fgjid的记录
    clear_fgjid(jid);

    UnMaskAll();

}

static void sigini_pid(JobItem *jobitem){
    if(jobitem->state == S_RUNNING || jobitem->state == S_SUSPENDED){
        kill(jobitem->pid, SIGINT);
        jobitem->state = S_ABORTED;
    }
}

//ctrl c
void sigini_handler(int signum){
    printf("sigini_handler\n");
    MaskAll();

    int jid = get_fgjid();

    if(jid < 0){
        UnMaskAll();
		return; //此时没有前台作业
    }
    
    //将job中的都abort
    process_job(jid, sigini_pid);

    //由于是ctrl c，直接退出就行
    rm_byjid(jid);

    UnMaskAll();

    //TODO 不确定是否要在这里删除。。因为后面进程退出应该还有一次发送才对。。
    //直接删除该job
    // rm_byjid(jid);
}


//TODO 有个问题 会不会恢复之后全部都是running状态。。
static void sigcont_pid(JobItem *jobitem){
    //SIGCONT让对应的job恢复执行 负数则是处理给对应的进程组
    if(jobitem->state == S_SUSPENDED && (kill(jobitem->pid, SIGCONT) == 0)){
        jobitem->state = S_RUNNING;
    }
}

sigset_t g_MaskAll, g_MaskNone;
sigset_t g_MaskOld;
void init_signmask(){
    sigfillset(&g_MaskAll);
    sigemptyset(&g_MaskNone);
    // sigaddset(&mask_one, SIGCHLD);
}
void MaskAll(){
    sigprocmask(SIG_BLOCK, &g_MaskAll, NULL);
}
void UnMaskAll(){
    sigprocmask(SIG_UNBLOCK, &g_MaskAll, NULL);
}


//job前台执行
void fg_signal(int jid){
    //首先对于jid中的process发送SIGCONT
    // int job_pgid = get_jobpgid(jid);
    // int ret = kill(-job_pgid, SIGCONT);
    process_job(jid, sigcont_pid);

    //设置JID
    set_fgjid(jid);
}

//用于fg阻塞操作
// void fg_wait(int jid){
//     // 进程回收不需要做，只要等待前台进程就行
//     sigset_t mask_temp;
//     sigemptyset(&mask_temp);
//     // 设定不阻塞任何信号
//     // 其实可以直接sleep显式等待信号
//     while (get_fgjid() == jid)
//         sleep(0);
//     	// sigsuspend(&mask_temp);
//     return;
// }
void fg_wait(int jid){
    while (jid == get_fgjid()){
        sleep(0);
    }
    return;
}

//后台执行信号
void bg_signal(int jid){
    //由于转到bg的可能是暂停的进程，所以需要设置信号
    // int job_pgid = get_jobpgid(jid);
    // int ret = kill(-job_pgid, SIGCONT);
    process_job(jid, sigcont_pid);
    clear_fgjid(jid);
}

static void sigkill_pid(JobItem *jobitem){
    //SIGCONT让对应的job恢复执行 负数则是处理给对应的进程组
    if((jobitem->state != S_KILLED && jobitem->state != S_ABORTED)  && (kill(jobitem->pid, SIGKILL) == 0)){
        jobitem->state = S_RUNNING;
    }
}

void killjid_signal(int jid, int signal){
    // int job_pgid = get_jobpgid(jid);
    // int ret = kill(-job_pgid, SIGKILL);
    process_job(jid, sigkill_pid);
}
void killpid_signal(int pid, int signal){
    if(signal == SIGKILL){
        kill(-pid, SIGKILL);
    }
}
