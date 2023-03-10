#include<unistd.h>   
#include<stdio.h>
int main()   
{
    // char *argv[ ]={"ls", "-al", "/etc/passwd", NULL};   
    char *argv[ ]={"jobs", "-l", NULL};   
    // char *envp[ ]={"PATH=/bin", NULL};
    char *envp[ ]={NULL};
    // execve("/bin/ls", argv, envp);   

    char *tmp[1];
    tmp[0] = NULL;
    // int ret = execve("jobs", argv, envp);
    // int ret = execlp("jobs", "-l", NULL);
    int ret = execlp("pwd", "", NULL);
    // int ret = execv("jobs", argv, envp);
    // printf("%d", ret);
    // execve("jobs", argv, envp);   
}
