#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <signal.h> // kill(2)
#include <string.h> 	// strcmp, etc.
#include "sandbox.h"  	// Mine
#include <dlfcn.h>


extern char **get_launchd_endpoints(int);

void *g_sblibHandle = NULL;


void usage()
{
	fprintf(stderr,"Johnny's Sandbox Utility, v0.1\n");
	fprintf(stderr,"Usage: %s _pid_ [what]...\n", getprogname());
	fprintf(stderr,"Where: what = all:     All operations (not ready yet)\n");
	fprintf(stderr,"              mach:    Check all known mach ports\n");
	fprintf(stderr,"              file:    Check file access (Requires dir or file argument)\n");
	fprintf(stderr,"              inspect: Call syscall_inspect on this PID\n");
	fprintf(stderr,"              vtrace:  Call syscall_vtrace on this PID\n");

	fprintf(stderr,"\n");


}

int checkAll (pid_t Pid)
{

	int op = 0;
	while (operation_names[op])
	{
		int denied = sandbox_check (Pid, operation_names[op],  SANDBOX_CHECK_NO_REPORT, NULL);
		printf("%s: %s\n", operation_names[op], denied ? "deny" : "allow");
		op++;
	
	}


	return 0;
} 

int main(int argc, char **argv)
{


	// First open libsandbox.1.dylib. Yes, we're linked with it,
	// but No, versions change significantly between 3xx and 459+
	// If we can't find it, no biggy - the inspect feature will just be disabled.


	char *libsb = getenv ("LIBSB");
	if (!libsb) libsb = "libsandbox.1.dylib";
	g_sblibHandle = dlopen (libsb, RTLD_GLOBAL);


	if (argc < 2)  { usage(); exit(1);}

	// If first argument is a PID, go for sandbox_check

	uint32_t pid = 0;
	int rc = 0;

	char container[1024];
	char *path = "/tmp";

	rc = sscanf (argv[1], "%d", &pid);

	if (rc != 1)
	{
		fprintf(stderr,"Expecting a PID as the first argument. \"%s\" sure as heck isn't\n",
			argv[1]);
		usage();
		exit(5);
	}

	char buf[4096];

	// Does PID exist? a simple check
	rc = kill (pid, 0);

	if (rc < 0) { fprintf (stderr, "%d: No such process\n", pid); exit(2);}
	// Do a quick check to see if the process is at all sandboxed
	
	if (pid)
	{
	   rc  = sandbox_check (pid, NULL, 0);
	    if (rc == 0) { fprintf(stderr,"Process %d is not sandboxed. All further checks are moot (everything allowed).\n",pid); 
		if (strcmp(argv[2], "inspect")) exit(0);
			}
	


	   // If we have a PID, maybe it is contained?
	   container[0] = '\0';
	   int len = 0x339;
	   uint64_t pid64 = pid;


	   rc = sandbox_container_path_for_pid (pid, container, 1024);
	   if (rc == 0) {
			  printf("PID %d Container: %s\n", pid, container);
			}
	   else  printf("PID %d is sandboxed, but not containerized\n",pid);
	 
	}


	// If no more arguments, then we're done.

	if (argc == 2) { return 0; };

	if (argc == 3) { 


		if (strcmp(argv[2],"file") == 0)
			{
				fprintf(stderr,"file: options requires an argument (file or folder to check)\n");
				exit(5);
			}

#ifdef SB459
		if (strncmp(argv[2], "vtrace", strlen(argv[2])) == 0)
		{
			int rc = sandbox_vtrace_enable();
			fprintf(stderr,"RC: %d\n",rc);
			char *rep = sandbox_vtrace_report();
			fprintf(stderr,"rep: %p\n",rep);
			exit(rc);
		}

#endif
		if (strncmp(argv[2], "inspect", strlen(argv[2])) == 0)
		{
			
			//int rc =  __sandbox_ms("Sandbox",0x10, &pid2,pid2,buf, 4096);
			//printf("RC: %d, Buf: %s\n", rc,buf);

		typedef int	sbip_func(int, char **buf, int *size);

		sbip_func  *sandbox_inspect_pid = dlsym(g_sblibHandle, "sandbox_inspect_pid");
		if (sandbox_inspect_pid) {
			int size = 0;
			char *buf = NULL;
			rc = sandbox_inspect_pid(pid,&buf,&size);

			if (rc == 0) fprintf(stdout,"%s\n", buf);
			else
			 fprintf(stderr,"sandbox_inspect_pid failed (RC: %d)\n", rc);
			}
		else { fprintf (stderr," Cant find sandbox_inspect_pid in libsandbox.\nLikely iOS version too old or too new (though you can try __sandbox_ms (0x10) directly)\n"); }



		}

		if (strcmp(argv[2],"mach") == 0)
		{
			fprintf(stderr,"Checking Mach services for %d....\n",pid);

			char **services = get_launchd_endpoints(1);

	
			int s = 0;
			for (s = 0;
			     services[s];
			     s++)
			{
			int mach_check = sandbox_check (pid, 
				"mach-lookup", 
				 SANDBOX_FILTER_RIGHT_NAME | SANDBOX_CHECK_NO_REPORT,
				 services[s]);
	
			printf("%s: %s\n", services[s], mach_check ?"Nope": "Yep");
			}
			return 0;

		}
	
		if (strcmp(argv[2], "all") == 0) { exit(checkAll(pid)); }

		path = realpath(argv[2],NULL); 

			}

	// Check if the path exists, otherwise sandbox_check will fail (for the wrong reason)

	if (strcmp(argv[2],"file") == 0) 
	{
		path = argv[3];

		if (!path) {  fprintf(stderr,"Path %s doesn't exist\n", argv[2]);  exit(1); } 

		int read_denied = sandbox_check (pid, "file-read-data",  SANDBOX_FILTER_PATH  | SANDBOX_CHECK_NO_REPORT, path);
		int write_denied = sandbox_check (pid, "file-write-data", SANDBOX_FILTER_PATH | SANDBOX_CHECK_NO_REPORT, path);

		// I like verbose messages. And tertiary operators. Even if nested
		printf("The sandbox is %s restricting PID %d from reading %s writing to %s\n", 
			read_denied  ? "" :"not",
			pid,
			write_denied ? (read_denied ? "and" : "but is restricting") :
				       (read_denied ? "but not restricting" : "or"),
			path);

		// For those of you wanting to script this, the return value says all
		exit ( (read_denied ? 0x10 : 0) + (write_denied ? 0x20 : 0));

	}
}