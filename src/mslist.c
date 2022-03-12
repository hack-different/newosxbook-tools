#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/kern_memorystatus.h>





#if 0
// from kern_memorystatus.h
// OS X definition
typedef struct memorystatus_priority_entry {
        pid_t pid;
        int32_t priority;
        uint64_t user_data;
        int32_t limit;
        uint32_t state;
} memorystatus_priority_entry_t;
#endif

#ifdef IOS6 
// iOS 6.x definition

typedef struct memorystatus_priority_entry_ios6 {
        pid_t pid;
        uint32_t flags;
        int32_t hiwat_pages;
        int32_t priority;
        int32_t reserved;
        //int32_t reserved2;
} memorystatus_priority_entry_ios6_t;

#define memorystatus_priority_entry memorystatus_priority_entry_ios6
#endif

#define NUM_ENTRIES 1024

char *state_to_text(int State)
{

        // Convert kMemoryStatus constants to a textual representation
        static char returned[80];

        sprintf (returned, "0x%02x ",State);

        if (State & kMemorystatusSuspended) strcat(returned,"Suspended,");
        if (State & kMemorystatusFrozen) strcat(returned,"Frozen,");
        if (State & kMemorystatusWasThawed) strcat(returned,"WasThawed,");
        if (State & kMemorystatusTracked) strcat(returned,"Tracked,");
        if (State & kMemorystatusSupportsIdleExit) strcat(returned,"IdleExit,");
        if (State & kMemorystatusDirty) strcat(returned,"Dirty,");

        if (returned[strlen(returned) -1] == ',')
             returned[strlen(returned) -1] = '\0';

        return (returned);

}


int main (int argc, char **argv)
{

        struct memorystatus_priority_entry memstatus[NUM_ENTRIES];
        size_t  count = sizeof(struct memorystatus_priority_entry) * NUM_ENTRIES;

        // call memorystatus_control
        int rc = memorystatus_control (MEMORYSTATUS_CMD_GET_PRIORITY_LIST,    // 1 - only supported command on OS X
                                       0,    // pid
                                       0,    // flags
                                       memstatus, // buffer
                                       count); // buffersize

        if (rc < 0) { perror ("memorystatus_control"); exit(rc);}

        int entry = 0;
        for (; rc > 0; rc -= sizeof(struct memorystatus_priority_entry))
        {
#ifdef IOS6 

                printf ("PID: %5d\tFlags: %x\tHiWat Pages:%2d\tReserved:%d\n",
                        memstatus[entry].pid,
                        memstatus[entry].priority,
                        memstatus[entry].hiwat_pages,
                        memstatus[entry].reserved);
                
                
#else
                printf ("PID: %5d\tPriority:%2d\tUser Data: %llx\tLimit:%2d\tState:%s\n",
                        memstatus[entry].pid,
                        memstatus[entry].priority,
                        memstatus[entry].user_data,
                        memstatus[entry].limit,
                        state_to_text(memstatus[entry].state));

#endif
                entry++;


        }



}