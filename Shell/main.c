/* $begin shellmain */
#include "csapp.h"
#define MAXARGS   128

/* function prototypes */
void eval(char* cmdline);
int parseline(char* buf, char** argv);
int builtin_command(char** argv);
char* splitine(char* subCmd, char* tmpCmdline, char* tmpSubCmd);

int main()
{
	char cmdline[MAXLINE]; /* 保存命令行 */

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
		if ((pid = Fork()) == 0) {   /* Child runs user job */

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
			int status;
			if (waitpid(pid, &status, 0) < 0)
				unix_error("waitfg: waitpid error");
		}
		else
			printf("%d %s", pid, cmdline);
	}
	return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char** argv)
{
	if (!strcmp(argv[0], "quit")) /* quit command */
		exit(0);
	if (!strcmp(argv[0], "&"))    /* Ignore singleton &(发生于& & & ...) */
		return 1;
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
	/* 执行 */
	if (tmpSubCmd[strlen(tmpSubCmd) - 1] != '\n')
		strcat(tmpSubCmd, "\n");

	return subCmd;
}
/* end splitline */
