

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//建变量表
//读取变量位置
//支持变量修改


//引用有环境变量设置的全局变量
extern char ** environ;

#define MAXVARS 200
#define MAXVARLEN 256
//表格
struct VarItem{
    char isGrobal;
    // char *varName;
    char *addr;
};
// typedef struct VarItem g_VLTable[MAXVARS];

struct VarItem g_VLTable[MAXVARS];
int VarItemNum = 0;

//读取表
int VLenviron2Table(){
    char ** envir = environ;
    int tableIdx = 0;
    while(*envir)
    {
        char *line = *envir;
        // char *pos  = strchr(line, '=');
        struct VarItem *item = &g_VLTable[tableIdx];
        item->isGrobal = 1;
        // item.varName = malloc((pos-line+2)*sizeof(char));
        // strncpy(item.varName, line, pos-line+1);
        // item.varName[pos-line+1] = '\0';
        item->addr = line;
        envir++; 
        tableIdx++;
    }
    VarItemNum = tableIdx;
    return 1;
}


static char *getVLByNameUnSafe(const char *name){
    for(int i = 0; i < VarItemNum; i++){
        if(strncmp(g_VLTable[i].addr, name, strlen(name)) == 0){
            return g_VLTable[i].addr + strlen(name) + 1;
        }
    }
    return NULL;
}

//读取变量信息
const char * getVLByName(const char *name){
    return getVLByNameUnSafe(name);
}

static int addNewVarItem(const char* varName, const char *value, char isGrobal){
    // if(VarItemNum >= MAXVARS)
        // exit();
    // char *newVar = malloc((strlen(varName)+strlen(value)+2)*sizeof(char));
    char *newVar = malloc(MAXVARLEN*sizeof(char));
    int pos = 0;
    for(int i = 0; varName[i]!='\0';i++,pos++)
        newVar[pos] = varName[i];
    newVar[pos++] = '=';
    for(int i = 0; value[i]!='\0';i++,pos++)
        newVar[pos] = value[i];
    g_VLTable[VarItemNum].isGrobal = isGrobal; 
    g_VLTable[VarItemNum].addr = newVar; 
    VarItemNum++;
    return 1;
}
static int setVLByName(const char* varName, const char *value, char isGrobal){
    if(strlen(value) > MAXVARLEN)
        return 0;

    char *pos = getVLByNameUnSafe(varName);
    if(pos == NULL){
        printf("hello\n");
        return addNewVarItem(varName, value, isGrobal);
    }
    else{
        strcpy(pos,value);
        return 1;
    }
}

int setGlobalVLByName(const char* varName, const char *value){
    return setVLByName(varName, value, 1);
}
int setLocalVLByName(const char* varName, const char *value){
    return setVLByName(varName, value, 0);
}

void traverseVLTable(void (*func)(char, char*)){
    for(int i = 0; i < VarItemNum; i++){
        func(g_VLTable[i].isGrobal, g_VLTable[i].addr);
    }
}

void traverse(){
    char ** envir = environ;
    while(*envir)
    {
        fprintf(stdout,"%s\n",*envir);
        envir++;
    }
}

static void traverseSimple(char isGrobal, char*addr){
    printf("%d %s\n", isGrobal, addr);    
}