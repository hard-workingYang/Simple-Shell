#include "job.h"
#include <stdio.h>

int main()
{
    init_jobs();
    printf("finish init_jobs\n");

    //add test
    add_job(1000, S_RUNNING, "running test 1");
    add_job(1001, S_RUNNING, "running test 2");
    add_job(1002, S_RUNNING, "running test 3");
    add_job(1003, S_RUNNING, "running test 4");

    add_p2job(1, 1005, S_SUSPENDED, "stopped test 1");
    add_p2job(1, 1006, S_SUSPENDED, "stopped test 1");
    add_p2job(2, 1007, S_SUSPENDED, "stopped test 1");
    printf("finish add test\n");

    //traverse test 
    list_jobs();
    printf("finish traverse test\n");

    //look up test
    int ret = pid2jid(1002);
    printf("expect the ret is %d, and the ret value is %d\n", 2, ret);
    printf("finish pid2jid test\n");
    
    //change state test
    chg_state_bypid(1001, S_DONE);
    chg_state_byjid(1, S_DONE);
    printf("changed state:\n");
    list_jobs();
    printf("finish change state test\n");

    //rm jid test
    printf("\n");
    printf("the job %%2 should be removed\n");
    rm_byjid(2);
    list_jobs();
    printf("\n");
    add_job(1010, S_RUNNING, "running test 5");
    printf("the new job's jid should be 2, and have a + tag.\n");
    printf("the jod of jid 3 shoule have a - tag.\n");
    list_jobs();
    printf("finish rm job test\n");

    //check job end test
    for(int i = 0; i < 4; i++)
        printf("%d\n", check_jobend(i));
    printf("finish check job end test\n");

    //bg test
    
}