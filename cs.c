#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

 int csops(pid_t pid, uint32_t ops, void *useraddr, size_t usersize) {

	// In this part. case we dont need this since csops is in libsystem_kernel.. .but....
	return (syscall(169, pid, ops, useraddr, usersize));

}


int main (int argc, char **argv) {

	// simple csops command-line driver.

	if (argc  < 3) { fprintf(stderr,"Usage: %s _op_ _pid_\n", argv[0]); exit(1);}

        uint32_t op = atoi(argv[1]);
	pid_t pid =  atoi(argv[2]);

    
	char buf[4096] = { 0 };
	uint32_t bufsize = 4096;

	int rc = csops (pid, op, buf, bufsize);
	if ( rc == -1) {  return 1; }
	// if here then ok:

	write (1, buf, bufsize);
	

}