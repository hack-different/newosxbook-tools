#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h> // fchmod
#include "../include/sandbox.h"
#include "../include/sandbox.h" // My sandbox.h

typedef int bool;
#include <dlfcn.h>

void usage(void)
{
	
	fprintf(stderr, "Usage: sandbox-exec [options] command [args]\nOptions:\n  -f profile-file     Read profile from file.\n  -n profile-name     Use pre-defined profile.\n  -p profile-string   Specify profile on the command line.\n  -D key=value        Define a profile parameter.\n -t trace_file    Trace operations to trace_file\n Exactly one of -f, -n, -p must be specified.\n");

	exit(0x40);

} // usage


int debug = 0;
#define dprintf if (debug) printf

int main (int argc, char **argv)
{

	debug = (getenv ("JDEBUG") != NULL);
	// This is as close as possible to the decompilation of OS X's (10.11.4) sandbox-exec
	// including calling sandbox_create_params() before processing arguments. 

	sbProfile_t *compiled_profile;

	char *err = NULL;

	sbParams_t *params=sandbox_create_params();

	if (!params) { fprintf(stderr,"Can't create params!\n"); exit(1);}

	if (argc < 2) { usage(); }



	int opt;

	char *profile = NULL;
	int cmd = 0;
	char *profileName = NULL;
	char *profileString = NULL;
	char *tracePath = NULL;

	while ((opt = getopt(argc,argv,"D:c:de:f:n:p:t:")) > -1)
	  {
		switch (opt)
		{
			case 'f':  profile = optarg; break;
			case 'n':  profileName = optarg; break;
			case 'p':  profileString = optarg; break;
			case 't':  tracePath = optarg; break;
		
			default: usage();
	
			
		
		}

	  }

	cmd = optind;
	
	if (tracePath && profileName)
	{
		fprintf(stderr, "tracing isn't implemented for named profiles; use -f or -p to specify a profile\n");		
		exit(0x40);
	}

	//compiled_profile = sandbox_compile_entitlements ("no-internet", params, &err);


	if (profile) compiled_profile = sandbox_compile_file (profile, params, &err);

	// The built-in profiles:
	// ----------------------
	// kSBXProfileNoInternet (no-internet)
	// kSBXProfileNoWriteExceptTemporary (no-write-except-temporary)
	// kSBXProfileNoWrite         (no-write)
	// kSBXProfileNoNetwork       (no-network)
	// kSBXProfilePureComputation (pure-computation)

	if (profileName) compiled_profile = sandbox_compile_named (profileName, params, &err);
	if (profileString) compiled_profile = sandbox_compile_string (profileString, params, &err);

	//sandbox_set_param (params, "x", "y");
	if(!compiled_profile) { fprintf(stderr, "No compiled profile. Error: %s\n", err); exit(2); } 


	int dump= 1;
	if (dump && compiled_profile->blob)
	{
	fprintf(stderr,"Profile: %s, Blob: %p Length: %d\n",
			(compiled_profile->name? compiled_profile->name : "(custom)" ), 
                         compiled_profile->blob, compiled_profile->len);
	int fd = open("/tmp/out.bin", O_WRONLY | O_TRUNC| O_CREAT);
	fchmod (fd, 0666);
	write(fd, compiled_profile->blob, compiled_profile->len);
	fprintf(stderr,"dumped compiled profile to /tmp/out.bin\n");
	}

	int flags = 0;
	int rc = 0;
	if (tracePath) { 
#ifdef SB459
		
		rc = sandbox_set_trace_path(compiled_profile,tracePath); 
#else
		void *sblibhandle = dlopen ("libsandbox.dylib", RTLD_GLOBAL);

		typedef int sstp(void *, char *);
		sstp * sandbox_set_trace_path = dlsym (sblibhandle, "sandbox_set_trace_path");

		if (!sandbox_set_trace_path)
		fprintf(stderr,"Warning: Tracing not supported in this sandbox version (can't get set_trace_path - %p)\n", sblibhandle);
		else {
			rc = sandbox_set_trace_path(compiled_profile,tracePath); 

		}
#endif
		if (rc == 0) fprintf(stderr,"Tracing to %s\n", tracePath);
		else fprintf(stderr,"Tracing error - Unable to trace to %s\n", tracePath);

	} 


	fprintf(stderr,"Applying container\n");
	rc =  sandbox_apply_container (compiled_profile, flags);

	if (rc != 0) { perror("sandbox_apply_container"); }
	
	fflush(NULL);
	if (compiled_profile) sandbox_free_profile(compiled_profile);
	fprintf (stderr, "EXECING %s\n", argv[optind]);
	execvp (argv[optind], argv+optind);

	perror("execvp");


}