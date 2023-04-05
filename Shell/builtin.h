#ifndef __BUILTIN_H__
#define __BUILTIN_H__

# include "env.h"
int builtin_cd(char** des);
int builtin_jobs(char** argv);

int builtin_bg(char** argv);
int builtin_fg(char** argv);
int builtin_kill(char** argv);


#endif /* __CSAPP_H__ */

