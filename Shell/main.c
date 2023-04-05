/* $begin shellmain */
#include "csapp.h"
#include "builtin.h"
#include "job.h"
#include "signal.h"
#define MAXARGS   128
#define MAXCMDLEN 256

/* function prototypes */
void eval(char* cmdline);
int parseline(char* buf, char** argv);
int builtin_command(char** argv);
char* splitine(char* subCmd, char* tmpCmdline, char* tmpSubCmd);
int deal_re_in_out(const char** argv);

void init() {
	// 读取系统环境变量
	VLenviron2Table();

	//jobs对象初始化
	init_jobs();

	init_signmask();

	//淘汰了，因为不能一次把所有后台子进程都退出，实际上只会退出前台的job
	//忽略退出的两个信号
	// Signal(SIGINT, SIG_IGN); //ctrl c
	// Signal(SIGQUIT, SIG_IGN);

	//要处理一下子进程的退出情况
	Signal(SIGCHLD, sigchld_handler);
	//ctrl-z信号处理
	Signal(SIGTSTP, sigtstp_handler);
	//ctrl-c信号处理
	Signal(SIGINT, sigini_handler);

}

int main()
{
	char cmdline[MAXLINE]; /* 保存命令行 */

	dup2(1, 2);

	init();

	while (1) {
		/* 读入 */
		char* pwdvar = getenv("PWD");
		printf("%s> ", pwdvar);
		Fgets(cmdline, MAXLINE, stdin);  // csapp对fgets做了保护措施的实现
		if (feof(stdin))
			exit(0);
		
		char* subCmd = NULL;
		char tmpCmdline[MAXLINE];
		char tmpSubCmd[MAXLINE];
		strcpy(tmpCmdline, cmdline);
		while ((subCmd = splitine(subCmd, tmpCmdline, tmpSubCmd)) != NULL) {
			eval(tmpSubCmd);
		}
	}
}
/* $end shellmain */

/* If first arg is a builtin command, run it and return true */
int builtin_command(char** argv)
{
	if (!strcmp(argv[0], "quit")) /* quit command */
		exit(0);
	else if (!strcmp(argv[0], "&"))    /* Ignore singleton &(发生于& & & ...) */
		return 1;
	else if (!strcmp(argv[0], "cd"))
		return builtin_cd(argv);
	else if (!strcmp(argv[0], "jobs"))
		return builtin_jobs(argv);
	else if (!strcmp(argv[0], "bg"))
		return builtin_bg(argv);
	else if (!strcmp(argv[0], "fg"))
		return builtin_fg(argv);
	else if (!strcmp(argv[0], "kill")) //严格来说不算是内置的？但中间的一些实现要修改一下
		return builtin_kill(argv);
	return 0;                     /* Not a builtin command */
}

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char* buf, char** argv)
{

	// 检测输入输出重定向符号

	char* delim;         /* Points to first space delimiter(第一个空格分隔符的指针) */
	int argc;            /* Number of args */
	int bg;              /* Background job? */

	buf[strlen(buf) - 1] = ' ';  /* Replace trailing '\n' with space */
	while (*buf && (*buf == ' ')) /* Ignore leading spaces(忽略前导空格) */
		buf++;

	/* Build the argv list */
	argc = 0;
	while ((delim = strchr(buf, ' '))) {  /* strchr() 用于查找字符串中的一个字符，并返回该字符在字符串中第一次出现的位置 */
		argv[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while (*buf && (*buf == ' ')) /* Ignore spaces */
			buf++;
	}
	argv[argc] = NULL;

	if (argc == 0)  /* Ignore blank line */
		return 1;
}
/* $end parseline */

int deal_re_in_out(const char **argv) 
{
	int argc = 0;
	int truncation = 0;

	while (argv[argc] != NULL)
		argc++;

	truncation = argc;

	while (--argc >= 0) {
		if (strcmp(argv[argc], ">") == 0 && argv[argc + 1] != NULL) {	//输出重定向
			int fd_t;
			fd_t = Open(argv[argc + 1], O_WRONLY | O_CREAT, 0644);
			Dup2(fd_t, STDOUT_FILENO);
			Close(fd_t);
			truncation = argc;
		}
		if (strcmp(argv[argc], "<") == 0 && argv[argc + 1] != NULL) {	//输入重定向
			int fd_t;
			fd_t = Open(argv[argc + 1], O_RDONLY, 0644);
			Dup2(fd_t, STDIN_FILENO);
			Close(fd_t);
			truncation = argc;
		}
	}

	return truncation;
}

void eval(char* cmdline)
{
	char* argv[MAXARGS]; /* Argument list execve() */
	char buf[MAXLINE];   /* Holds modified command line */
	int bg;              /* Should the job run in bg or fg? */
	pid_t pid;           /* Process id */

	strcpy(buf, cmdline);

	if (strlen(buf) == 0)
		return;

	// 处理 & 
	/* Should the job run in the background? */
	while (buf[strlen(buf)-1] == ' ')
		buf[strlen(buf) - 1] = 0;
	if ((bg = (buf[strlen(buf) - 1] == '&')) != 0)
		buf[strlen(buf) - 1] = 0;

	if (strlen(buf) == 0)
		return;

	// 处理 | 
	// 先在buf里处理管道符，若有，则记录管道符数量 + 1个子进程，维护它们的buf结构，等待后续创建
	char subJob[MAXARGS][MAXLINE];
	int jobNums = 0;
	if (strchr(buf, '|') != NULL) {
		char* tmpJob;
		tmpJob = strtok(buf, "|"); // 第一次调用需要传入待分割的字符串和分隔符
		while (tmpJob != NULL) {
			strcpy(subJob[jobNums++], tmpJob);
			tmpJob = strtok(NULL, "|");
		}
	}

	// 执行到这行，subJob子程序保存着所有要创建的命令行程序以及参数，包括重定向符
	int step = 0;
	int last_fd[2];
	int fd[2];
	while (step++ < jobNums) {
		if (pipe(fd) < 0) {
			perror("pipe");
			exit(1);
		}
		parseline(subJob[step - 1], argv);

		if (argv[0] == NULL)
			return;   /* Ignore empty lines */

		if (!builtin_command(argv)) {
			pid = Fork();
			if (pid == 0) {   /* Child runs user job */
				if (step == 1) {
					Close(fd[0]);
					Dup2(fd[1], STDOUT_FILENO);
					Close(fd[1]);
				}
				else if(step == jobNums){
					Close(last_fd[1]);
					Dup2(last_fd[0], STDIN_FILENO);
					Close(last_fd[0]);
				}
				else {
					Close(last_fd[1]);
					Dup2(last_fd[0], STDIN_FILENO);
					Close(last_fd[0]);
					Close(fd[0]);
					Dup2(fd[1], STDOUT_FILENO);
					Close(fd[1]);
				}

				// 处理程序的输入输出重定向
				argv[deal_re_in_out(argv)] = NULL;

				/* 想执行命令需要从环境目录中进行，这里将execve直接替换为execvp */
#ifdef __ZTH__USING_PATH
				if (execve(argv[0], argv, environ) < 0)   /* environ为linux全局变量，代表系统环境变量，定义在unistd.h */
					int i = 0;
				while (environ[i] != NULL)
					printf("%s\n", environ[i++]);
#else
				if (execvp(argv[0], argv) < 0)
#endif
				{
					printf("%s: Command not found.\n", argv[0]);
					exit(0);
				}
			}
			else {	//父进程
				if (step > 1) {	//第一步不关管道
					Close(last_fd[1]); // 关闭管道写端
					Close(last_fd[0]); // 关闭管道读端
				}
				last_fd[0] = fd[0];
				last_fd[1] = fd[1];
			}

			/* Parent waits for foreground job to terminate */
			if (!bg) {
				int status;
				if (waitpid(pid, &status, 0) < 0)
					unix_error("waitfg: waitpid error");
			}
			else
				printf("%d %s", pid, cmdline);
		}
	}
	//可执行到此，只能是父进程，父进程关闭所有管道
	if (jobNums > 0) {
		Close(fd[0]);
		Close(fd[1]);
	}


	// 为了无管道的命令不出错，先复制过来
	if (jobNums == 0) {
		parseline(buf, argv);

		if (argv[0] == NULL)
			return;   /* Ignore empty lines */

		if (!builtin_command(argv)) {
			//fork前后保护一下
			MaskAll();
			if ((pid = Fork()) == 0) {   /* Child runs user job */

				//TODO 需要补充一个是环境变量的 VLtable2environ

				UnMaskAll();
				setpgid(0, 0);
				//子进程中恢复默认处理方式
				Signal(SIGCHLD, SIG_DFL);
				Signal(SIGTSTP, SIG_DFL);
				Signal(SIGINT, SIG_DFL);

				// 处理程序的输入输出重定向
				argv[deal_re_in_out(argv)] = NULL;

				/* 想执行命令需要从环境目录中进行，这里将execve直接替换为execvp */
#ifdef __ZTH__USING_PATH
				if (execve(argv[0], argv, environ) < 0)   /* environ为linux全局变量，代表系统环境变量，定义在unistd.h */
					int i = 0;
				while (environ[i] != NULL)
					printf("%s\n", environ[i++]);
#else
				if (execvp(argv[0], argv) < 0)
#endif
				{
					printf("%s: Command not found.\n", argv[0]);
					exit(0);
				}
			}

			/* Parent waits for foreground job to terminate */
			if (!bg) {
				//这里需要添加一下对应进程的信息
				int jid = add_job(pid, S_RUNNING, buf);
				set_fgjid(jid);
				UnMaskAll();

				fg_wait(jid);
				// int status;
				// if (waitpid(pid, &status, 0) < 0)
					// unix_error("waitfg: waitpid error");
			}
			else {
				int jid = add_job(pid, S_RUNNING, buf);
				printf("[%d] %d\n", jid, pid);
				UnMaskAll();
				// printf("%d %s", pid, cmdline);
			}
		}
	}
	return;
}

/* begin splitline */
char* splitine(char * subCmd, char * tmpCmdline, char *tmpSubCmd)
{
	if (subCmd == NULL)
		subCmd = strtok(tmpCmdline, ";"); // 第一次调用需要传入待分割的字符串和分隔符
	else
		subCmd = strtok(NULL, ";");

	if (subCmd != NULL)
		strcpy(tmpSubCmd, subCmd);

	if (tmpSubCmd[strlen(tmpSubCmd) - 1] != '\n')
		strcat(tmpSubCmd, "\n");

	return subCmd;
}
/* end splitline */