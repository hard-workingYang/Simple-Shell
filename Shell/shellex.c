#include "csapp.h"


#define MAXARGS   128

/* Ignore spaces */
#define IGNORE_SPACE(buf) while (*buf && (*buf == ' ')) buf++; 


int isRunning = 0;
pid_t g_pid;


void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
        /* Read */
        printf("> ");                   
        fgets(cmdline, MAXLINE, stdin); 
        if (feof(stdin))
            exit(0);

        /* Evaluate */
        eval(cmdline);
    } 
}

void signalHandler(int signalNum){
    // bool isRunning = false;

    if(isRunning){
        kill(g_pid, SIGINT);
    }
    
}

/* eval - Evaluate a command line */
void eval(char *cmdline) {
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);

    //解析命令行
    bg = parseline(buf, argv); 
    if (argv[0] == NULL)  
	    return;   /* Ignore empty lines */

    signal(SIGINT, signalHandler);
    signal(SIGTSTP, signalHandler);

    if (!builtin_command(argv)) { 
        // 
        if ((pid = fork()) == 0) {   /* Child runs user job */
            // if (execve(argv[0], argv, environ) < 0) {
            if (execvp(argv[0], argv) < 0) {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }
        /* Parent waits for foreground job to terminate */
        if (!bg) {
            int status;
            g_pid = pid;
            isRunning = 1;
            if (waitpid(pid, &status, 0) < 0)
                unix_error("waitfg: waitpid error");
            isRunning = 0;
            
        }
        else
            printf("%d %s", pid, cmdline);
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) {
    char *tmp[1];
    if (!strcmp(argv[0], "quit")) /* quit command */
	    exit(0);  
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	    return 1;
    if (!strcmp(argv[0], "jobs"))
        tmp[0] = NULL;
        execve("jobs", tmp, environ);

    return 0;                     /* Not a builtin command */
}
/* $end eval */


/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) {
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    //外面使用的是strcpy，所以最后一个一定是\n，这里替换成空格
    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */

    IGNORE_SPACE(buf)
    // while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	// buf++;

    /* Build the argv list */
    argc = 0;
    //剩下的空格就分隔了参数项 这里就逐项处理然后存储到argv中
    while ((delim = strchr(buf, ' '))) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;

        IGNORE_SPACE(buf)
        // while (*buf && (*buf == ' ')) /* Ignore spaces */
        //     buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	    return 1;

    //如果是有&来进行后台执行的话，&一定是在最后，判断一下
    //下面的=和==操作值得学习
    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	    argv[--argc] = NULL;

    return bg;
}
/* $end parseline */

