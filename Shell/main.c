/* $begin shellmain */
#include "csapp.h"
#include "builtin.h"
#include "job.h"
#include "signal.h"

#define MAXCMDLEN 256
/* function prototypes */
void eval(char* cmdline);
int parseline(char* buf, char** argv);
int builtin_command(char** argv);
char* splitine(char* subCmd, char* tmpCmdline, char* tmpSubCmd);

void init(){
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

/* $begin eval */
/* eval - 执行一条命令行 */
void eval(char* cmdline)
{
	char* argv[MAXARGS]; /* Argument list execve() */
	char buf[MAXLINE];   /* Holds modified command line */
	int bg;              /* Should the job run in bg or fg? */
	pid_t pid;           /* Process id */

	strcpy(buf, cmdline);
	bg = parseline(buf, argv);
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

			/* 想执行命令需要从环境目录中进行，这里将execve直接替换为execvp */
#ifdef __ZTH__USING_PATH
			if (execve(argv[0], argv, environ) < 0) {  /* environ为linux全局变量，代表系统环境变量，定义在unistd.h */
				int i = 0;
				while (environ[i] != NULL)
					printf("%s\n", environ[i++]);
#else
			if (execvp(argv[0], argv) < 0){
#endif
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
		else{
			int jid = add_job(pid, S_RUNNING, buf);
			printf("[%d] %d\n", jid, pid);
			UnMaskAll();
			// printf("%d %s", pid, cmdline);
		}
	}
	return;
}

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
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char* buf, char** argv)
{
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

	/* Should the job run in the background? */
	if ((bg = (*argv[argc - 1] == '&')) != 0)
		argv[--argc] = NULL;

	return bg;
}
/* $end parseline */

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

