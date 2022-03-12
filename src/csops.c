
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define CSOPS_NUM	169
int csops (pid_t pid, uint32_t ops, user_addr_t useraddr, user_size_t usersize)
{

	int rc = syscall (CSOPS_NUM, pid, ops, useraddr, usersize);
	return rc;


}


int main (int argc, char **argv) {

	
	 int pid = atoi(argv[1]);
	 int op  = atoi (argv[2]);

	 char buf[4096];
	 int bufSize = 4096;

	 int rc = csops(pid, op, buf, bufSize);
	 if (rc < 0 ) // -1
		{
				perror("csops");
				exit(1);
		}

	 write(2, buf, bufSize);




}