
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<signal.h>    

void unix_error(char *msg);

extern char **environ; /* defined by libc */

/* Misc constants */
#define	MAXLINE	 8192  /* max text line length */

