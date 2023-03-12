#include <stdio.h>
#include "builtin.h"

#define PATH_MAX 256
#define EXIT_FAILURE -1

int builtin_cd(char **des)
{
	char** p = des;
	int nums = 0;
	while (*(p++) != NULL)
		nums++;

	if (nums < 2)
		return 1;
	else if (nums > 2) {
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
}