#include <stdlib.h> // for malloc
#include <stdio.h> 
#include <string.h>
#include <sys/sysctl.h>
#include <errno.h>

/*** 
 * OBSOLETE - AAPL invalidated the stack_snapshot system call (#365) in favor
 *    of micro_stackshot[with_config] (#492/491) in XNU 3248 and later
 *    
 ***/

//typedef unsigned int uint32_t;
//typedef unsigned long long uint64_t;

#define STACKSHOT_TASK_SNAPSHOT_MAGIC 0xdecafbad
#define STACKSHOT_THREAD_SNAPSHOT_MAGIC 0xfeedface
#define STACKSHOT_MEM_SNAPSHOT_MAGIC  0xabcddcba
// This is new in ML, apparently. Looks like this API ain't going away anytime soon!
#define STACKSHOT_MEM_AND_IO_SNAPSHOT_MAGIC     0xbfcabcde 


// The following are from osfmk/kern/thread.h
#define TH_WAIT                 0x01                    /* queued for waiting */
#define TH_SUSP                 0x02                    /* stopped or requested to stop */
#define TH_RUN                  0x04                    /* running or on runq */
#define TH_UNINT                0x08                    /* waiting uninteruptibly */
#define TH_TERMINATE    0x10                    /* halted at termination */
#define TH_TERMINATE2   0x20                    /* added to termination queue */

#define TH_IDLE                 0x80                    /* idling processor */


char *stateToText (int32_t state)
{
   static char stateText[160];
   // using snprintf and strncat to avoid linking with _chk variants
   snprintf(stateText,80,"%d - ", state);
   if (state & TH_WAIT) strncat (stateText , "waiting ",80);
   if (state & TH_SUSP) strncat (stateText , "suspended ",80);
   if (state & TH_RUN) strncat (stateText , "running ",80);
   if (state & TH_UNINT) strncat (stateText , "Uninterruptible ",80 );
   if (state & TH_TERMINATE) strncat (stateText , "Halted at termination ",80);
   if (state & TH_TERMINATE2) strncat (stateText , "Added to termination queue ",80);
   if (state & TH_IDLE) strncat (stateText , "Idling processor ",80);
   return (stateText);

}

#pragma pack(0)

#ifdef LP64
struct frame {
        void *retaddr;
	void *fp;
};
struct uframe {
        void *retaddr;
	void *fp;
};
#else
struct uframe {
        void *retaddr;
	void *fp;
};
struct frame {
	uint32_t retaddr;
	uint32_t fp;
};
#endif


// Ripped from osfmk/kern/debug.h

struct mem_and_io_snapshot {
        uint32_t        snapshot_magic;
        uint32_t        free_pages;
        uint32_t        active_pages;
        uint32_t        inactive_pages;
        uint32_t        purgeable_pages;
        uint32_t        wired_pages;
        uint32_t        speculative_pages;
        uint32_t        throttled_pages;
        int                     busy_buffer_count;
        uint32_t        pages_wanted;
        uint32_t        pages_reclaimed;
        uint8_t         pages_wanted_reclaimed_valid; // did mach_vm_pressure_monitor succeed?
} __attribute__((packed));

#ifndef LION
struct task_snapshot {
        uint32_t                snapshot_magic;
        int32_t                 pid;
        uint32_t                nloadinfos;
        char                    ss_flags;
        /* We restrict ourselves to a statically defined
         * (current as of 2009) length for the
         * p_comm string, due to scoping issues (osfmk/bsd and user/kernel
         * binary compatibility).
         */
        char                    p_comm[17];

} __attribute__ ((packed));

struct thread_snapshot {
        uint32_t                snapshot_magic;
        uint32_t                nkern_frames;
        uint32_t                nuser_frames;
        uint64_t                wait_event;
        uint64_t                continuation;
        uint64_t                thread_id;
        int32_t                 state;
        char                    ss_flags;
} __attribute__ ((packed));

#else

struct thread_snapshot {
        uint32_t                snapshot_magic;
        uint32_t                nkern_frames;
        uint32_t                nuser_frames;
        uint64_t                wait_event;
        uint64_t                continuation;
        uint64_t                thread_id;
        uint64_t                user_time;
        uint64_t                system_time;
        int32_t                 state;
        char                    ss_flags;
        // Struct appears to be off by 8. These are always 0. Possibly reserved? Need to check..
	uint32_t		res1;  
	uint32_t		res2;
} __attribute__ ((packed));


struct task_snapshot {
        uint32_t                snapshot_magic;
        int32_t                 pid;
        uint32_t                nloadinfos;
        uint64_t                user_time_in_terminated_threads;
        uint64_t                system_time_in_terminated_threads;
        int                             suspend_count;
        int                             task_size;    // pages
        int                             faults;         // number of page faults
        int                             pageins;        // number of actual pageins
        int                             cow_faults;     // number of copy-on-write faults
        char                    ss_flags;
        /* We restrict ourselves to a statically defined
         * (current as of 2009) length for the
         * p_comm string, due to scoping issues (osfmk/bsd and user/kernel
         * binary compatibility).
         */
        char                    p_comm[17];
} __attribute__ ((packed));
#endif


#pragma pack()

int stack_snapshot(int pid, char *tracebuf, int bufsize, int options)
{

/* Inputs:           uap->pid - process id of process to be traced, or -1
 *                   for the entire system 
 *                   uap->tracebuf - address of the user space destination
 *                   buffer
 *                   uap->tracebuf_size - size of the user space trace buffer
 *                   uap->options - various options, including the maximum
 *                   number of frames to trace.
 */


	return syscall (365, pid, tracebuf, bufsize, options);
}


int dump_mem_and_io_snapshot(struct  mem_and_io_snapshot *mais)
{

   printf ("Pages: Free: %-8d        Active:    %-8d  Purgeable: %-8d Wired: %-8d\n",
		mais->free_pages, mais->active_pages, mais->purgeable_pages, mais->wired_pages,
		mais->speculative_pages);
   printf ("       Speculative: %-8d Throttled: %-8d  Wanted:    %-8d Reclaimed: %-8d\n",
		mais->speculative_pages, mais->throttled_pages, mais->pages_wanted, mais->pages_reclaimed);
	
   return (0);
}

int dump_thread_snapshot(struct thread_snapshot *ths)
{

   if (ths->snapshot_magic != STACKSHOT_THREAD_SNAPSHOT_MAGIC)
	{	
		fprintf(stderr,"Error: Magic %p expected, Found %p\n", STACKSHOT_TASK_SNAPSHOT_MAGIC, ths->snapshot_magic);
		return (0);
	}
    printf ("\tThread ID: 0x%x ", ths->thread_id) ; 
    printf ("State: %s\n" ,  stateToText(ths->state));
    if (ths->wait_event) printf ("\tWaiting on: 0x%x ", ths->wait_event) ; 
    if (ths->continuation) {
    printf ("\tContinuation: %p\n", ths->continuation);

	}

	return (ths->nkern_frames + ths->nuser_frames);
}

void dump_task_snapshot(struct task_snapshot *ts)
{
   if (ts->snapshot_magic != STACKSHOT_TASK_SNAPSHOT_MAGIC)
	{	
		fprintf(stderr,"Error: Magic %p expected, Found %p\n", STACKSHOT_TASK_SNAPSHOT_MAGIC, ts->snapshot_magic);
		return;
	}

   fprintf(stdout, "PID: %d (%s) Flags: %x \n", ts->pid, ts->p_comm,ts->ss_flags);

}
int main (int argc, char **argv)
{
    // Have to supply a really large buffer - else we'll get error 28
    // Since this is a proof of concept, just put 500,000 - that oughtta do
    // the trick. The right way would be to malloc/realloc an increasingly larger
    // buffer while failing

#define BUFSIZE 500000

    char buf[BUFSIZE];
    int rc = stack_snapshot(-1,  // All processes
			    buf, // Save to buffer
			    BUFSIZE, // Duh
			    100);    // # of frames to trace

    struct task_snapshot *ts;
    struct thread_snapshot *ths;
    int off = 0;
    int warn = 0;
    int nframes = 0;
    int is64bit = 0;

    

    if (rc <0) { perror ("stack_snapshot"); return (-1); }

    
    
    while (off< rc) { 

	int x;
     ts = (struct task_snapshot *) (buf + off);
     ths = (struct thread_snapshot *) (buf + off);
	
     switch (ts->snapshot_magic)
	{
	   case STACKSHOT_MEM_AND_IO_SNAPSHOT_MAGIC:
		dump_mem_and_io_snapshot(ts);
		off += sizeof(struct mem_and_io_snapshot);
		break;
	   case STACKSHOT_TASK_SNAPSHOT_MAGIC:
		printf ("OFF: %d\n", off);
	        dump_task_snapshot(ts);
		if (ts->ss_flags & 0x1) is64bit = 1; else is64bit = 0;

	        off+= sizeof(struct task_snapshot);
		warn = 0;
		break;
	   case STACKSHOT_THREAD_SNAPSHOT_MAGIC:
		printf ("OFF: %d\n", off);
		nframes = dump_thread_snapshot(ths);
	        off+= sizeof(struct thread_snapshot);
	

    if (ths->nkern_frames || ths->nuser_frames)
	{
		
              printf ("\tFrames:    %d kernel %d user\n", ths->nkern_frames, ths->nuser_frames);
		// User first 
		while (ths->nuser_frames)
		{	
	  	    // This could be a 32-bit frame or a 64-bit frame
		    struct uframe *f = (struct uframe *) (buf + off);
		    printf ("\t\t%p\t%p\n", f->retaddr, f->fp);
		    ths->nuser_frames--;
		    off += (is64bit)? 16 : 8;
		}

		while (ths->nkern_frames)
		{	
	  	    // This could be a 32-bit frame or a 64-bit frame
		    struct frame *f = (struct frame *) (buf + off);
		    printf ("\t\t%p\t%p\n", f->retaddr, f->fp);
		    ths->nkern_frames--;
		    off += 16;
		}
	}

		warn = 0;
		break;
	   case STACKSHOT_MEM_SNAPSHOT_MAGIC:
		printf ("MEM magic\n");
		break;
	   default: 
		if (!warn) {
		warn++;
		fprintf(stdout, "Magic %p at offset %d? Seeking to next magic\n", ts->snapshot_magic, off);}
		 off++;;


        } // end switch
    
    } // end while
}