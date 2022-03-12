#include <stdio.h>
#include <dispatch/dispatch.h>
#include <pthread.h>

int main (int arg,char **argv)
{
    // Using pthread_self() inside a block will show you the thread it
    // is being run in. The interested reader might want to dispatch
    // this block several times, and note that the # of threads can
    // change according to GCD's internal decisions..

    void (^myblock1) (void) = ^ { printf(" Blocks are cool - \n");};
    dispatch_queue_t q = 
       dispatch_queue_create("com.technologeeks.demoq",  // Our name
                             DISPATCH_QUEUE_CONCURRENT); // DISPATCH_QUEUE_SERIAL or CONCURRENT

    dispatch_group_t g = dispatch_group_create();
 
    dispatch_group_async(g, q, myblock1);
    dispatch_group_async(g, q, myblock1);
        
    int rc= dispatch_group_wait(g, DISPATCH_TIME_FOREVER);
                                        
}