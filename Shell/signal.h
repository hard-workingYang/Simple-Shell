#ifndef __SIGNAL_H__
#define __SIGNAL_H__

void init_signmask();
void sigchld_handler(int signum);
void sigtstp_handler(int signum);
void sigini_handler(int signum);

void MaskAll();
void UnMaskAll();

void fg_wait(int jid);
void fg_signal(int jid);
void bg_signal(int jid);

void killjid_signal(int jid, int signal);
void killpid_signal(int pid, int signal);

#endif