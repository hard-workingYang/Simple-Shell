#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "builtin.h"
#include "job.h"
#include "signal.h"

#define PATH_MAX 256
// #define EXIT_FAILURE -1
static int argcCount(char **argv){
	char** p = argv;
	int nums = 0;
	while (*(p++) != NULL)
		nums++;
	return nums;
}

int builtin_cd(char **des)
{
	int argc = argcCount(des);
	if (argc < 2)
		return 1;
	else if (argc > 2) {
		printf("cd: too many arguments\n"); //模拟一下，不定义统一接口了
		return 1;
	}

    if (des[1][0] != '/') {
		char cwd[PATH_MAX], path[PATH_MAX];

		// 获取当前工作目录
		if (getcwd(cwd, sizeof(cwd)) == NULL) {
			perror("getcwd");
			exit(EXIT_FAILURE);
		}
		// 拼接路径
		if (sprintf(path, "%s/%s", cwd, des[1]) < 0) {
			perror("sprintf");
			exit(EXIT_FAILURE);
		}
		chdir(path);
		getcwd(cwd, sizeof(cwd));
		// 获取绝对路径的目录部分
		setGlobalVLByName("PWD", cwd);
    }
	else {
		chdir(des[1]);     // 完成cd命令
		setGlobalVLByName("PWD", des[1]);
	}
	
	// 更改pwd
	//setGlobalVLByName("OLDPWD", getVLByName("PWD"));
	return 1;
}

int builtin_jobs(char **argv){
	return list_jobs();
}


//测试之后，发现fg/bg后面多少个参数都没问题，而且独立处理
//比如：bg %4 %6 那么两个就会分开处理
//bg %4 abc 那么这里就会执行bg %4然后bg abc输出job not found
//而如果bg后面没有参数，那么就是对最新的那个job进行处理，也就是jobs后有+的那个处理

static int fg_one(int jid){
	//屏蔽其他信号
	MaskAll();

	//验证jid在不在
	//查找对应的job
	if(!check_jidAvail(jid)){
		UnMaskAll();
		printf("fg: %%%d: no such job\n", jid);
		return 0;
	}

	//现在就要把后台job抬出来处理
	//发送信号启动后台的进程
	fg_signal(jid);

	//解除信号阻塞？但此时要是刚好这个组完蛋了就没了。。
	//但不阻塞的话，谁能修改jid呢？还是靠的信号
	//现在想到可能的解决方案是：只阻塞SIGCHLD，避免现在这个进程死掉？
	//但想来这个组要是在解除之后完蛋了，那么应该也没事，就是fg不能保持了嘛
	//似乎没有大的问题
	UnMaskAll();

	//开始阻塞
	fg_wait(jid);

	return 1;
}

//fg也可以多参数，但是会在第一个挂到前台的时候发生阻塞，第一个转后台的时候第二个才会转前台
//观察了一下，如果+号job程因为suspend在后台，那么bg之后+号job和-号job的加减号会对调
int builtin_fg(char** argv){
	int argc = argcCount(argv);
	if(argc == 1){
		//如果没有参数则默认对于+号job进行处理
		fg_one(get_latestJobid());
	} else{
		//顺序处理，依次阻塞对象		
		for(int i = 1; i < argc; i++){
			if(argv[i][0] == '%'){
				int jid = atoi(argv[i] + 1);
				if(jid < 0) {
					fprintf(stderr, "%s\n", "illegal JID.");
					return 0;
				}
				fg_one(jid);
			}
		}
	}
	return 1;
}

static int bg_one(int jid){
	//屏蔽其他信号
	MaskAll();

	//验证jid在不在
	//查找对应的job
	if(!check_jidAvail(jid)){
		UnMaskAll();
		return 0;
	}
	printf("bg_one jid %d\n", jid);
	//现在就要把后台job抬出来处理
	//发送信号启动后台的进程
	bg_signal(jid);

	UnMaskAll();

	return 1;
}

int builtin_bg(char** argv){
	int argc = argcCount(argv);

	if(argc == 1){
		//如果没有参数则默认对于+号job进行处理
		bg_one(get_latestJobid());
	} else{
		//顺序处理，依次阻塞对象		
		for(int i = 1; i < argc; i++){
			if(argv[i][0] == '%'){
				int jid = atoi(argv[i] + 1);
				if(jid < 0) {
					fprintf(stderr, "%s\n", "illegal JID.");
					return 0;
				}
				bg_one(jid);
			}
		}
	}
	return 1;
}


//TODO 这是阉割版的kill 
//     现在一定要求有三个参数，而且只发送 SIGKILL信号。。
int builtin_kill(char** argv){
	int argc = argcCount(argv);
	if(argc == 2){
		printf("kill: too many arguments\n"); //模拟一下，不定义统一接口了

	} else if(argc == 3){
		if(argv[1][0] == '%'){
			int jid = atoi(argv[1] + 1);
			if(jid < 0) {
				fprintf(stderr, "%s\n", "illegal JID.");
				return 0;
			}
			killjid_signal(jid, SIGKILL);
			//
		}else{
			int pid = atoi(argv[1]);
			if(pid < 0){
				fprintf(stderr, "%s\n", "illegal PID.");
				return 0;
			}
			killpid_signal(pid, SIGKILL);
		}
	} else{
		printf("kill: too many arguments\n"); //模拟一下，不定义统一接口了
	}
	return 1;
}