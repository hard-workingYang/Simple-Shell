#include <stdio.h>

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

	// 完成cd命令
	chdir(des[1]);

	// 更改pwd
	//setGlobalVLByName("OLDPWD", getVLByName("PWD"));
	setGlobalVLByName("PWD", des[1]);
}