#include <stdio.h>
#include <unistd.h>

int main()
{
    char buf[256];
    while(1){
        scanf("%s", buf);
        printf("%s", buf);
    }
}