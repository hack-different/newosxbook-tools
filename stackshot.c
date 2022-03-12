//#include "stackshot.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <kern/kcdata.h> // for the KCData format
#include <time.h>
#include <unistd.h>
#include <uuid/uuid.h>

/**
 * Stackshot for 10.11/9 (XNU 32xx) and later:
 *
 * Uses the new (and still undocumented) syscall #491 in place of #365 which
 * has been removed.
 *
 * Obviously requires root access to run. Provides full stack traces - both
 * kernel and user - of all threads in the system.
 *
 * Compiles cleanly for iOS and MacOS, so I'm leaving it as source rather
 * than provide a binary.
 *
 *
 * (AAPL will likely slap an entitlement on this in MacOS 13/iOS 11 anyway..
 * Use/copy freely while you can - but a little acknowledgment wouldn't hurt :-)
 *   
 * The full explanation of how to use this is in MOXiI Vol. I (debugging)
 * The full explanation of how it works (kernel-side, pretty darn ingenious)
 *   is in MOXiI Vol. II
 *
 * So stay tuned. More coming.
 *
 * Jonathan Levin, 11/01/2016
 *
 */

// As with the previous version, we have to rip the typedefs and structs from the kernel
// headers, as they are "private" and not exported...

typedef struct stackshot_config {
        /* Input options */
        int             sc_pid;                 /* PID to trace, or -1 for the entire system */
        uint32_t        sc_flags;               /* Stackshot flags */
        uint64_t        sc_delta_timestamp;     /* Retrieve a delta stackshot of system state that has changed since this time */
        
        /* Stackshot results */
        uint64_t        sc_buffer;              /* Pointer to stackshot buffer */
        uint32_t        sc_size;                /* Length of the stackshot buffer */
        
        /* Internals */ 
        uint64_t        sc_out_buffer_addr;     /* Location where the kernel should copy the address of the newly mapped buffer in user space */   
        uint64_t        sc_out_size_addr;       /* Location where the kernel should copy the size of the stackshot buffer */
} stackshot_config_t;


// As well as the definition of the system call itself, which is a wrapper over the generic syscall(2)

int stack_snapshot_with_config(int stackshot_config_version, stackshot_config_t *stackshot_config, uint32_t stackshot_config_size) {

  // AAPL "deprecates" syscall(2) in 10.12. And I say, if you deprecate it, at least expose the 
  // syscall header to user mode! 
  return (syscall (491, stackshot_config_version, (uint64_t) stackshot_config, stackshot_config_size));

}


// from kern/debug.h
enum {
        STACKSHOT_GET_DQ                           = 0x01,
        STACKSHOT_SAVE_LOADINFO                    = 0x02,
        STACKSHOT_GET_GLOBAL_MEM_STATS             = 0x04,
        STACKSHOT_SAVE_KEXT_LOADINFO               = 0x08,
        STACKSHOT_GET_MICROSTACKSHOT               = 0x10,
        STACKSHOT_GLOBAL_MICROSTACKSHOT_ENABLE     = 0x20,
        STACKSHOT_GLOBAL_MICROSTACKSHOT_DISABLE    = 0x40,
        STACKSHOT_SET_MICROSTACKSHOT_MARK          = 0x80,
        STACKSHOT_ACTIVE_KERNEL_THREADS_ONLY       = 0x100,
        STACKSHOT_GET_BOOT_PROFILE                 = 0x200,
        STACKSHOT_SAVE_IMP_DONATION_PIDS           = 0x2000,
        STACKSHOT_SAVE_IN_KERNEL_BUFFER            = 0x4000,
        STACKSHOT_RETRIEVE_EXISTING_BUFFER         = 0x8000,
        STACKSHOT_KCDATA_FORMAT                    = 0x10000,
        STACKSHOT_ENABLE_BT_FAULTING               = 0x20000,
        STACKSHOT_COLLECT_DELTA_SNAPSHOT           = 0x40000,
	// and there's more, but I don't use them..
};


#define STACKSHOT_CONFIG_TYPE 1

// Convenience function hiding the implementation - in case AAPL changes it yet again
int get_stackshot (pid_t Pid, char **Addr, uint64_t *Size, uint64_t Flags)
{

    stackshot_config_t	stconf = { 0 };
    stconf.sc_pid =  Pid;  
    stconf.sc_buffer = 0; // buf;
    stconf.sc_size   =  0; // 4096;
    stconf.sc_flags  = STACKSHOT_KCDATA_FORMAT | Flags;
 
    stconf.sc_out_buffer_addr = (uint64_t) Addr;  
    stconf.sc_out_size_addr =  (uint64_t) Size;
  
    int rc = stack_snapshot_with_config(STACKSHOT_CONFIG_TYPE, &stconf, sizeof(stconf));
    return (rc);
} // get_stackshot


// Dumpers: These will format the item into the Output string, which may be just 
// printed out, or used further (as I do in Process Explorer)
void dump_mem_and_io_stats (struct mem_and_io_snapshot *MaI, char *Output)
{

 	sprintf(Output, "Pages: %d free, %d active, %d Inactive, %d Purgeable\n       %d Wired, %d Speculative, %d Throttled, %d Filebacked\nCompressor: %d comp, %d decomp, %d size\nBusy: %d\n",
		MaI->free_pages,
		MaI->active_pages,
		MaI->inactive_pages,
		MaI->purgeable_pages,
		MaI->wired_pages,
		MaI->speculative_pages,
		MaI->throttled_pages,
		MaI->filebacked_pages,	
		MaI->compressions,
		MaI->decompressions,
		MaI->compressor_size,
        	MaI->busy_buffer_count);
}
		 

void dump_iostats(struct io_stats_snapshot *Stats, char *Output)
{

	sprintf(Output, "Disk Reads: %lld reads (%lld bytes), %lld writes (%lld bytes)\n",
		Stats->ss_disk_reads_count, Stats->ss_disk_reads_size,
		Stats->ss_disk_writes_count, Stats->ss_disk_writes_size);

} // dump_iostats

void dump_cpu_times (struct stackshot_cpu_times *Times, char *Output)
{ 
	sprintf (Output,"User: %lld.%06lld secs, System: %lld.%06lld secs",
		Times->user_usec / 1000000,
		Times->user_usec % 1000000,
		Times->system_usec / 1000000,
		Times->system_usec % 1000000);

}


void dump_array (kcdata_item_t	Item, char *Output)
{

   int elemType = (Item->flags >> 32) & UINT32_MAX;
   int elemCount = (Item->flags) & UINT32_MAX;

    //printf("%d elements, Type %x\n", elemCount, elemType);

   char *elemData = Item->data;
   int elem = 0;
   int adv = 0;

    
    for (elem = 0 ; elem < elemCount; elem++)
    {
    switch (elemType)
	{
		case STACKSHOT_KCTYPE_USER_STACKFRAME64:
		case STACKSHOT_KCTYPE_KERN_STACKFRAME64:
			{
				if (elem == 0) sprintf(Output,"\t\t%s Stack Trace:\n",
					elemType == STACKSHOT_KCTYPE_USER_STACKFRAME64 ? "User" : "Kernel");
				
				adv = sizeof (struct stack_snapshot_frame64);
				struct stack_snapshot_frame64 *ssf64 =
				  (struct stack_snapshot_frame64 *) elemData;
				sprintf(Output+strlen(Output),"\t\tLR: 0x%016llx, SP: 0x%016llx\n", ssf64->lr, ssf64->sp);
				break;
			}

		case KCDATA_TYPE_LIBRARY_LOADINFO64:
			{
				if (elem == 0) sprintf(Output,"\t\tLoaded Libraries:\n");

				char uuidUnparsed[128];
				adv = sizeof(struct dyld_uuid_info_64);
				struct dyld_uuid_info_64 *dui64 = 
				(struct dyld_uuid_info_64 *) elemData;
				uuid_unparse_upper (dui64->imageUUID, uuidUnparsed);
				sprintf(Output + strlen(Output),"\t\t0x%llx - %s\n",
					dui64->imageLoadAddress, uuidUnparsed);




			}
				break;

		case  STACKSHOT_KCTYPE_DONATING_PIDS: //0x907u         /* int[] */ 
			{
				if (elem == 0) sprintf(Output,"\t\tImportance Donors: ");
				sprintf(Output + strlen(Output),"%d, ", (* (int *) elemData));
				adv = sizeof(int);

				if (elem ==elemCount -1) Output[strlen(Output)-2] = '\0'; // truncate last ,
			}
			break;

		default: printf("ARRAY OF TYPE %x - Unhandled (yet)\n", elemType);
			break;
	}
	elemData += adv;
    } // end for

}


char *ths_flags_to_text (uint64_t Flags)
{

	static char returned[4096];
	returned[0] ='\0';
        if (Flags & kHasDispatchSerial) strcat (returned, "Has dispatch serial, ");   //    = 0x4,

	// probably don't need this in output..
	 if  (Flags & kStacksPCOnly) strcat (returned, "PC only, ");   //         = 0x8,    /* Stacif (Flags & k traces have no frame pointers. */

        if (Flags & kThreadDarwinBG) strcat (returned, "DarwinBG, ");   //       = 0x10,   /* Thread is darwinbg */
        if (Flags & kThreadIOPassive) strcat (returned, "Passive I/O, ");   //      = 0x20,   /* Thread uses passive IO */
        if (Flags & kThreadSuspended) strcat (returned, "Suspended, ");   //      = 0x40,   /* Thread is suspended */
        if (Flags & kThreadTruncatedBT) strcat (returned, "Truncated BT, ");   //    = 0x80,   /* Unmapped pages caused truncated bacif (Flags & ktrace */
        if (Flags & kGlobalForcedIdle) strcat (returned, "Global Forced Idle, ");   //     = 0x100,  /* Thread performs global forced idle */
        if (Flags & kThreadFaultedBT) strcat (returned, "Faulted BT, ");   //      = 0x200,  /* Some thread stacif (Flags & k pages were faulted in as part of BT */
        if (Flags & kThreadTriedFaultBT) strcat (returned, "Tried Faulted BT, ");   //   = 0x400,  /* We tried to fault in thread stacif (Flags & k pages as part of BT */
        if (Flags & kThreadOnCore) strcat (returned, "on core, ");   //         = 0x800,  /* Thread was on-core when we entered debugger context */
        if (Flags & kThreadIdleWorker) strcat (returned, "Idle Worker, ");   //     = 0x1000, /* Thread is an idle libpthread worif (Flags & ker thread */
	if (returned[strlen(returned) -1] == ' ')
	returned[strlen(returned) - 2 ] = '\0';
	if (!returned[0] )
	sprintf(returned, "0x%llx", Flags);

	return (returned);

	

} ; // ths_flags_to_text 


const char *ths_state_to_text(uint32_t State)
{

	static char returned[64];
	returned[0] = '\0';
		if (State & SS_TH_WAIT)   strcat (returned,"Waiting ");
		if (State & SS_TH_SUSP)   strcat (returned,"Suspended ");
		if (State & SS_TH_RUN)    strcat (returned,"Running ");
		if (State & SS_TH_UNINT)  strcat (returned,"Uninterruptible ");
		if (State & SS_TH_IDLE)   strcat (returned,"Idle ");
		if (State & SS_TH_TERMINATE) strcat (returned,"Terminated ");
		if (State & SS_TH_TERMINATE2) strcat (returned,"will terminate ");

	if (!returned[0]) sprintf(returned, "0x%x", State);


	return (returned);
}

void dump_thread_snapshot (struct thread_snapshot_v3 *Ths, char *Output)
{

	
	sprintf(Output, "TID: %lld State: %s PRI: %d/%d Flags: %s ", 
			Ths->ths_thread_id,
			ths_state_to_text(Ths->ths_state),
			Ths->ths_sched_priority,
			Ths->ths_base_priority,
			Ths->ths_ss_flags?  ths_flags_to_text(Ths->ths_ss_flags) : "(none)");


	if (Ths->ths_continuation)
	{
	sprintf(Output + strlen(Output), "\n\t\tContinuation: 0x%llx (no kernel stack)", Ths->ths_continuation);
	}


} // dump_thread_snap

char *tsv2_flags_to_text (uint64_t Flags)
{

	static char returned[4096];
	returned[0] ='\0';

	if (Flags & kTaskRsrcFlagged) strcat (returned,  "Resources Flagged, ");
	if (Flags & kTerminatedSnapshot) strcat (returned,  "Terminated, ");
	if (Flags & kPidSuspended) strcat (returned,  "PID Suspended, ");
	if (Flags & kFrozen) strcat (returned,  "Frozen, ");
	if (Flags & kTaskDarwinBG) strcat (returned,  "DarwinBG, ");
	if (Flags & kTaskVisVisible) strcat (returned,  "Visible, ");
	if (Flags & kTaskVisNonvisible) strcat (returned,  "NonVisible, ");
	if (Flags & kTaskIsForeground) strcat (returned,  "Foreground, ");
	if (Flags & kTaskIsBoosted) strcat (returned,  "Boosted, ");
	if (Flags & kTaskIsSuppressed) strcat (returned,  "Suppressed, ");
	if (Flags & kTaskIsTimerThrottled) strcat (returned,  "Timer Throttled, ");
	if (Flags & kTaskIsLiveImpDonor) strcat (returned,  "Live Donor, ");
	if (Flags & kTaskIsImpDonor) strcat (returned,  "Donor, ");
	if (Flags & kTaskWqExceededConstrainedThreadLimit) strcat (returned,  "Wq Exc Const, ");
	if (Flags & kTaskWqExceededTotalThreadLimit) strcat (returned,  "Wq Exc Total, ");
	if (Flags & kTaskIsDirty) strcat (returned,  "Dirty, ");
	if (Flags & kTaskSharedRegionInfoUnavailable) strcat (returned,  "Shared Reg Unavail, ");
	if (Flags & kTaskWqFlagsAvailable) strcat (returned,  "WQ Flags avail, ");

	if (returned[strlen(returned) -1] == ' ')
	returned[strlen(returned) - 2 ] = '\0';
	return (returned);



} // tsv2_flags_to_text

// Main Item handler
void handle_kcdata_item (kcdata_item_t Item, char *Output)
{
	
	// Handling most KCData items - not all are needed for stack shots
	// some are for corpses (another fascinating topic in and of itself)
	// and you can always fill in support for the rest
	static int inContainer =0;

	{
		// print tabs only if not end
		int x = inContainer;
		if (Item->type == KCDATA_TYPE_CONTAINER_END) x--;
		while (x > 0) { sprintf (Output,"\t"); Output +=1; x--;}
	}

	switch (Item->type)
	{

	case KCDATA_TYPE_STRING_DESC: // 0x1u
		sprintf(Output,"String desc"); break;

	case KCDATA_TYPE_UINT32_DESC:// 0x2u
		sprintf(Output,"Uint32_t desc"); break;

	case KCDATA_TYPE_UINT64_DESC:// 0x3u
		sprintf(Output,"Uint64_t desc"); break;
	case KCDATA_TYPE_INT32_DESC: // 0x4u
		sprintf(Output,"Int32_t desc"); break;
	case KCDATA_TYPE_INT64_DESC: // 0x5u
		sprintf(Output,"Int64_t desc"); break;
	case KCDATA_TYPE_BINDATA_DESC:// 0x6u
		sprintf(Output,"bindata desc"); break;

	case KCDATA_TYPE_CONTAINER_BEGIN: // 0x13u

		// sprintf(Output,"Container begin"); 
		inContainer++; 
		break;
	case KCDATA_TYPE_CONTAINER_END: // 0x14u
		//sprintf(Output,"Container end"); 
		inContainer--; break;

	case  KCDATA_TYPE_ARRAY: // 0x11u         /* Array of data OBSOLETE DONT USE THIS*/
	case KCDATA_TYPE_ARRAY_PAD0: // 0x20u /* Array of data with: // 0 byte of padding*/
	case KCDATA_TYPE_ARRAY_PAD1: // 0x21u /* Array of data with 1 byte of padding*/
	case KCDATA_TYPE_ARRAY_PAD2: // 0x22u /* Array of data with 2 byte of padding*/
	case KCDATA_TYPE_ARRAY_PAD3: // 0x23u /* Array of data with 3 byte of padding*/
	case KCDATA_TYPE_ARRAY_PAD4: // 0x24u /* Array of data with 4 byte of padding*/
	case KCDATA_TYPE_ARRAY_PAD5: // 0x25u /* Array of data with 5 byte of padding*/
	case KCDATA_TYPE_ARRAY_PAD6: // 0x26u /* Array of data with 6 byte of padding*/
	case KCDATA_TYPE_ARRAY_PAD7: // 0x27u /* Array of data with 7 byte of padding*/
	case KCDATA_TYPE_ARRAY_PAD8: // 0x28u /* Array of data with 8 byte of padding*/
	case KCDATA_TYPE_ARRAY_PAD9: // 0x29u /* Array of data with 9 byte of padding*/
	case KCDATA_TYPE_ARRAY_PADa: // 0x2au /* Array of data with a byte of padding*/
	case KCDATA_TYPE_ARRAY_PADb: // 0x2bu /* Array of data with b byte of padding*/
	case KCDATA_TYPE_ARRAY_PADc: // 0x2cu /* Array of data with c byte of padding*/
	case KCDATA_TYPE_ARRAY_PADd: // 0x2du /* Array of data with d byte of padding*/
	case KCDATA_TYPE_ARRAY_PADe: // 0x2eu /* Array of data with e byte of padding*/
	case KCDATA_TYPE_ARRAY_PADf: // 0x2fu /* Array of data with f byte of padding*/

		dump_array(Item, Output);
		break;
	case  KCDATA_TYPE_TYPEDEFINTION: // 0x12u /* Meta type that describes a type on the fly. */
		sprintf(Output,"Type def\n");
		break;

	case KCDATA_TYPE_LIBRARY_LOADINFO: // 0x30u   /* struct dyld_uuid_info_32 */
		sprintf (Output,"Load Info\n"); break;

	case KCDATA_TYPE_LIBRARY_LOADINFO64: // 0x31u /* struct dyld_uuid_info_64 */
		sprintf (Output,"Load Info 64"); break;
	case KCDATA_TYPE_TIMEBASE: // 0x32u           /* struct mach_timebase_info */
	//	sprintf(Output,"Timebase: "); break;
		break;
	case KCDATA_TYPE_MACH_ABSOLUTE_TIME: // 0x33u /* uint64_t */
		sprintf (Output,"Abs Time: %lld", (*((uint64_t *) &Item->data[0])) ); break;
	case KCDATA_TYPE_TIMEVAL: // 0x34u            /* struct timeval64 */
		sprintf (Output,"Timeval: "); break;
	case KCDATA_TYPE_USECS_SINCE_EPOCH: // 0x35u  /* time in usecs uint64_t */
		{
		// Note that this type checks USECS. time_t (for ctime and friends) 
		// requires SECS. So adjust!
		time_t	secs_since_epoch = (*((uint64_t *) &Item->data[0])) / 1000000;
		sprintf (Output,"Time: %s..", ctime(&secs_since_epoch));

		break;
		}
	case KCDATA_TYPE_PID: // 0x36u                /* int32_t */
		sprintf (Output,"PID: \n"); break;
	case KCDATA_TYPE_PROCNAME: // 0x37u           /* char * */
		sprintf (Output,"procname: \n"); break;
	case KCDATA_TYPE_NESTED_KCDATA: // 0x38u      /* nested kcdata buffer */
		sprintf (Output,"kcdata: \n"); break;


	case  STACKSHOT_KCTYPE_IOSTATS: // 0x901u
		
		strcat (Output, "IOStats: ");
		dump_iostats ((void *)Item->data, Output + strlen(Output));

	

		break;
	case STACKSHOT_KCTYPE_GLOBAL_MEM_STATS: // 0x902u /* struct mem_and_io_snapshot */
		dump_mem_and_io_stats ((void *)Item->data, Output);
		break;
	case STACKSHOT_KCCONTAINER_TASK: // 0x903u:
		sprintf(Output,"Container task");

		break;
	
	case STACKSHOT_KCCONTAINER_THREAD: // 0x904u:
		sprintf(Output,"Container thread");
		break;

	case  STACKSHOT_KCTYPE_TASK_SNAPSHOT: //0x905u         /* task_snapshot_v2 */
		{
		//printf("Task snapshot: %d\n", Item->size);
		 struct task_snapshot_v2 *tsv2 = (struct task_snapshot_v2 *) Item->data;
		 sprintf(Output,"PID : %d (%lld), comm: %s Flags: %s\n", tsv2->ts_pid, tsv2->ts_unique_pid, tsv2->ts_p_comm,
				tsv2_flags_to_text(tsv2->ts_ss_flags));


		if (tsv2->ts_task_size < 1024 *1024)
		{
		sprintf(Output + strlen(Output), "\tSize: %lldK Max Res: %lldK", tsv2->ts_task_size / (1024), tsv2->ts_max_resident_size / (1024));

		}
		else
		sprintf(Output + strlen(Output), "\tSize: %lldM Max Res: %lldM", tsv2->ts_task_size / (1024*1024), tsv2->ts_max_resident_size / (1024*1024));
		 


		}
		break;
	case  STACKSHOT_KCTYPE_THREAD_SNAPSHOT: //0x906u       /* thread_snapshot_v2, thread_snapshot_v3 */
		{
		 dump_thread_snapshot ((void *)Item->data, Output);
		}
		break;

	case  STACKSHOT_KCTYPE_DONATING_PIDS: //0x907u         /* int[] */ // found in array
		sprintf(Output,"Donating PIDs");
		break;
	case  STACKSHOT_KCTYPE_SHAREDCACHE_LOADINFO: //0x908u  /* same as KCDATA_TYPE_LIBRARY_LOADINFO64 */
		{
		sprintf(Output,"Shared Cache Load Info");
				char uuidUnparsed[128];
				struct dyld_uuid_info_64 *dui64 = 
				(struct dyld_uuid_info_64 *) Item->data;
				uuid_unparse_upper (dui64->imageUUID, uuidUnparsed);
				sprintf(Output + strlen(Output),"\t\t0x%llx - %s\n",
					dui64->imageLoadAddress, uuidUnparsed);
		}
		break;
	case  STACKSHOT_KCTYPE_THREAD_NAME: //0x909u           /* char[] */
		sprintf(Output,"Thread Name: %s", Item->data);
		break;

	// These are only found in arrays, so I handle them in dump_array...
	case  STACKSHOT_KCTYPE_KERN_STACKFRAME: //0x90Au       /* struct stack_snapshot_frame32 */
		printf("Kernel Stack Frame\n");
	
		break;
	case  STACKSHOT_KCTYPE_KERN_STACKFRAME64: //0x90Bu     /* struct stack_snapshot_frame64 */
		printf("Kernel Stack Frame 64\n");
		break;
	case  STACKSHOT_KCTYPE_USER_STACKFRAME: //0x90Cu       /* struct stack_snapshot_frame32 */
		printf("User Stack Frame\n");
		break;
	case  STACKSHOT_KCTYPE_USER_STACKFRAME64: //0x90Du     /* struct stack_snapshot_frame64 */
		printf("User Stack Frame 64\n");
		break;

	// Back to common types:
	case  STACKSHOT_KCTYPE_BOOTARGS: //0x90Eu              /* boot args string */
		sprintf(Output,"Boot-Args: %s", Item->data);
		break;
	case  STACKSHOT_KCTYPE_OSVERSION: //0x90Fu             /* os version string */
	// printf("OSVer %d, 0x%llx\n", Item->size, Item->flags);
		sprintf(Output,"OS Version: %s", Item->data);
		break;
	case  STACKSHOT_KCTYPE_KERN_PAGE_SIZE: //0x910u        /* kernel page size in uint32_t */
		sprintf(Output,"Page size: %d bytes",   * ((uint32_t *)(Item->data)));

		break;
	case  STACKSHOT_KCTYPE_JETSAM_LEVEL: //0x911u          /* jetsam level in uint32_t */
		sprintf(Output,"Jetsam Level: %d",   * ((uint32_t *)(Item->data)));
		break;
	case  STACKSHOT_KCTYPE_DELTA_SINCE_TIMESTAMP: //0x912u /* timestamp used for the delta stackshot */
		sprintf(Output,"Delta since");
		break;

	case  STACKSHOT_KCTYPE_TASK_DELTA_SNAPSHOT: //0x940u   /* task_delta_snapshot_v2 */
		sprintf(Output,"Task Delta snapshot");
		break;
	case  STACKSHOT_KCTYPE_THREAD_DELTA_SNAPSHOT: //0x941u /* thread_delta_snapshot_v2 */
		sprintf(Output,"Thread Delta snapshot");
		break;

	case STACKSHOT_KCTYPE_KERN_STACKLR: //  0x913u          /* uint32_t */
		sprintf(Output,"Kern stack LR\n");
		break;

	case STACKSHOT_KCTYPE_KERN_STACKLR64: //  0x914u          /* uint64_t */
		printf("Kern stack LR 64\n");
		break;

	case STACKSHOT_KCTYPE_USER_STACKLR: //  0x915u          /* uint32_t */
		printf("Kern stack LR\n");
		break;

	case STACKSHOT_KCTYPE_USER_STACKLR64: //  0x916u          /* uint64_t */
		printf("Kern stack LR 64\n");
		break;

	case STACKSHOT_KCTYPE_NONRUNNABLE_TIDS: //  0x917u          /* uint64_t */
		printf("Nonrunnable TIDs\n");
		break;

	case STACKSHOT_KCTYPE_NONRUNNABLE_TASKS: //  0x918u          /* uint64_t */
		printf("Nonrunnable Tasks\n");
		break;

	case STACKSHOT_KCTYPE_CPU_TIMES: //  0x919u          /* struct stackshot_cpu_times */
		strcpy (Output, "CPU Times: ");
		dump_cpu_times((void *)Item->data,Output + strlen(Output));
		break;

	case STACKSHOT_KCTYPE_STACKSHOT_DURATION:   //  0x91au          /* struct stackshot_duration */
		printf("Stackshot duration\n");
		break;

	case STACKSHOT_KCTYPE_STACKSHOT_FAULT_STATS: // 0x91bu /* struct stackshot_fault_stats */
		printf("Stackshot fault stats\n"); 
		break;

	case STACKSHOT_KCTYPE_KERNELCACHE_LOADINFO: //  0x91cu /* kernelcache UUID -- same as KCDATA_TYPE_LIBRARY_LOADINFO64 */
		printf("kernelcache UUID:\n");
		break;

	case KCDATA_TYPE_BUFFER_END:
		printf("Buffer end!\n");
		break;
	default :
		printf("UNKNOWN MAGIC 0x%x! AAPL must've changed the format (or J hasn't gotten to it yet?)!\n", Item->type);
	

	}


} // handle_kcdata_item

void handle_kcdata_iter(kcdata_iter_t Iter,char *Output)
{
	handle_kcdata_item (Iter.item,Output);

};

int main (int argc, char **argv)
{

    char *addr;
    uint64_t size;
    int pid = -1;

    // Sorry , people. Stackshot requires root privileges :-|

    if (geteuid())  {
		fprintf(stderr,"You're wasting my time, little man. Come back as root.\n");
		exit(1);
	}

    if (argc  == 2)
	{
		pid = atoi(argv[1]);

	}

    int Flags = STACKSHOT_GET_GLOBAL_MEM_STATS | STACKSHOT_GET_DQ  | STACKSHOT_SAVE_LOADINFO | STACKSHOT_SAVE_KEXT_LOADINFO | STACKSHOT_SAVE_IMP_DONATION_PIDS;
   

    int rc = get_stackshot (pid, &addr, &size, Flags);


    if (rc < 0)  { 	
	perror ("stack_snapshot_with_config");
	exit(1);
		}

   // If you want the buffer for hexdumping, etc:
   // fprintf(stderr,"RC: %d - Got %d bytes in %p\n", rc, size, addr);
   // write (1, addr, size);


    kcdata_item_t item = (kcdata_item_t) addr;

    if (item->type != KCDATA_BUFFER_BEGIN_STACKSHOT) {
	fprintf(stderr,"Stackshot doesn't begin with magic - maybe AAPL changed format again?\n");
	return 1;
	}

	
    int off = 16; 

   int inContainer = 0;
 
   char Output[40960];
#ifdef APPLE_ITER
	// This loop will iterate using AAPL's own APIs:
	kcdata_iter_t	iter = kcdata_iter (addr, size);
	iter = kcdata_iter_next(iter);
	while (kcdata_iter_valid(iter) )
	{
	
// fprintf(stdout,"Type: 0x%x, Size: %d bytes is ", kcdata_iter_type(iter), kcdata_iter_size(iter));
		handle_kcdata_iter (iter,Output);
		printf("%s\n",Output);
		Output[0] ='\0';

#if 0
		if (kcdata_iter_type(iter) == KCDATA_TYPE_ARRAY)
		{
			if (kcdata_iter_array_valid(iter))
			printf(" (%d elements of type %x)\n", 
				kcdata_iter_array_elem_count(iter), kcdata_iter_array_elem_type(iter));
			else { printf("Invalid array!\n");}
		}
#endif

		iter = kcdata_iter_next(iter);
	}



#else // my way

    while (off < size)
    {
	int skip = 0;
    
   	kcdata_item_t item = (kcdata_item_t) (addr + off);

	
	handle_kcdata_item(item, Output);

	printf("%s\n", Output);
	Output[0] = '\0';
	int padding = item->size % 16;
 	if (padding ) padding = 16-padding;
	{
	//printf("Advancing by %d bytes, padding %d\n",  *(magic+1), padding);
	off+= item->size + 16 + padding;
	}
  
    }
    
#endif
    



} // main