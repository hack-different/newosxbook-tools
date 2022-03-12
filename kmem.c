#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h> // for fchmod
#include <mach/mach.h>

#include <mach/task.h>
#include <ctype.h>


// Sample memory dumper for processes (PID=...) or kernel proper (checkrain)
// Compile with gcc-arm64 ....c -o ....
// 
// and don't forget to sign:
// JENTS=com.apple.private.security.container-required,task_for_pid-allow,com.apple.system-task-ports jtool2 --sign /tmp/memdump
//
// As an exercise, you might want to implement mach_vm_write - to turn this into a kernel/process memory patcher :-)


// CAVEAT: Reading "wrong" addresses (e.g. kernel base, unslid) can and probably will lead to a panic  since
//         mach_vm_read() has no sanity checks in it. 
//
// Instead of including <mach/mach_vm.h> which would #error unsupported
// I include the relevant functions inlined here...


kern_return_t mach_vm_read
(
        vm_map_t target_task,
        mach_vm_address_t address,
        mach_vm_size_t size,
        vm_offset_t *data,
        mach_msg_type_number_t *dataCnt
);


void hexDump (void *buf, int len, uint64_t addr);


/////
int main (int argc, char **argv){


	if (argc < 3) {
		fprintf(stderr,"Usage: %s <address> <size>\n",
			argv[0]);
		exit(0);
		
	}


	// Assume that our target is a valid task port which we obtain
	// per checkra1n via task_for_pid

	// Also rig the pid here. (if you want this as an arbitrary user mode
	// reader - just get pid as argument, or..


	pid_t	pid = 0;


	// hack - you might want pid as an arg...

	if (getenv("PID")) { pid = atoi(getenv("PID")); }
	
	mach_port_t	target_task = MACH_PORT_NULL;

	kern_return_t kr = task_for_pid (mach_task_self(), // ipc_space
					 pid, // 0 or target
					&target_task);


	if (kr != KERN_SUCCESS) {
		fprintf(stderr,"Our method of getting target for PID %d doesn't work. Sorry\n", pid);
		return (1);

		}

	uint64_t addr ;
	uint32_t len ;

	len = atoi(argv[2]);
	int rc = sscanf (argv[1], "0x%llx", &addr);
	if (rc !=1 ) { fprintf(stderr," HELLO! Usage, man! USAGE!!!!\n"); return 2;}


	fprintf(stderr,"Reading %d bytes from 0x%llx\n", len, addr);

	vm_offset_t data;
	mach_msg_type_number_t dataCnt = 0;



	// FOR KERNEL MEMORY, we might need to dump a pointer that is UNSLID
	// Or one which is SLID.
	
	// #1: how do we tell difference? 
	//  (use getenv("SLIDE");

	if (pid == 0 && (getenv("SLIDE") != NULL)) {
	
		// #2: how do we get slide?
		//  - in MacOS - easy - we discussed SEVERAL KASLR leaks
		//  - in *OS - most JBs will leave the slide value somehow.

		// CHECKRAIN ONLY!
		// Luca made the astute observation that there is a task_info (..DYLD_...)
		// Which - on the kernel task - would make no sense
		// That's where he puts the slide value.

		struct task_dyld_info dyld_info = {};
		uint64_t slide = 0; 
                mach_msg_type_number_t  count =  sizeof(struct task_dyld_info);


                kern_return_t kr =  task_info (target_task, // task_inspect_t task,
                        TASK_DYLD_INFO, (task_info_t) &dyld_info, &count);



		if (kr != KERN_SUCCESS) {

			fprintf(stderr,"Can't get slide - are you using checkrain?\n");
			// too risky. Fail
			return(3);

		}
		else { 
                	slide = dyld_info.all_image_info_size;

			printf("SLIDE is 0%llx\n", slide);

			addr+=slide;
		}


	} // pid 0

	kr = mach_vm_read (target_task, //  vm_map_t target_task,
			   addr, // mach_vm_address_t address,
			   len, // mach_vm_size_t size,
			   &data, // vm_offset_t *data,
			   &dataCnt); //mach_msg_type_number_t *dataCnt

	// Dump memory!
	if (getenv("RAW") != NULL) {
		char filename[128];
		sprintf(filename, "/tmp/mem.pid.%d.0x%llx-0x%llx.bin",
				pid,
				addr, addr +len);
		int fd = open (filename, O_CREAT | O_TRUNC | O_WRONLY);
		fchmod (fd, 0644);
		int rc = write (fd, (void *)data, dataCnt);
		printf("RC of write was %d\n", rc);
		close(fd);
		}
	else 

	hexDump ((void *)data, dataCnt, addr);

	return 0;
}



//  Simple hexdump (from disarm.c)

void hexDump(void *Mem, int Len, uint64_t Addr)
{
    unsigned char *Buf = (unsigned char *)Mem;
    int i = 0;

    
    char formatStr[80];

    strcpy(formatStr,"%06llx");
    
    int rowBegin = 0;
    for (i = 0; i < Len; i++)
    {
        if ((i % 16) == 0) {
            if (i>0) {
                // Have to print previous line
                int j = 0;
		printf (" |");
                for (j = i - 16; j < i; j++)
                {
                    if (isprint(Buf[j])) printf("%c", Buf[j]);
                    else printf(".");
                }
		printf ("|\n");
            }
            if (1) { fprintf(stdout, "%08llx  ", //formatStr,
                                Addr +i); }
            
            rowBegin = i;
        }
        
        int rowPos = i%16;
        
        if ((rowPos == 0) || (rowPos == 8))
        {

           uint64_t Address = *((uint64_t *) (Buf + rowBegin + rowPos)) ;
            
            if  ((Address & 0xFFFFFFF000000000) == 0xFFFFFFF000000000)
            {
                // Kernel address
                
                printf("   %s0x%llx%s   ",
                       "", Address, "");
                
                
                i +=7; // and +1 when we leave loop
                continue;
            }
        }
        // If we're here, we want to print normally
    	if (rowPos == 8) printf (" ");
        printf ("%02x ",  Buf[i]);
        
    } // end for
    
    if ((i % 16) == 0) {
        if (i>0) {
            // Have to print previous line
            int j = 0;
		printf (" |");
            for (j = i - 16; j < i; j++)
            {
                if (isprint(Buf[j])) printf("%c", Buf[j]);
                else printf(".");
            }
		printf ("|\n");
        }
 }

}
