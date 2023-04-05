#ifndef __ENV_H__
#define __ENV_H__

//从全局变量读取到表
int VLenviron2Table();

//获取变量
const char * getVLByName(const char *name);

//设置全局变量
int setGlobalVLByName(const char* varName, const char *value);

//设置局部变量
int setLocalVLByName(const char* varName, const char *value);

//遍历内存中的变量表 参数是每一项的输出函数
void traverseVLTable(void (*func)(char, char*));

#endif