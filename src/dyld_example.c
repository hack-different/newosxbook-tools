#include <stdlib.h>
#include <unistd.h>
#define NULL	((void *) 0ULL)
#include <stdio.h>

// simple program that links two functions,
// calls one of them twice. <==

// - Understand the concepts behind dynamic linking
// - Understand "lazy binding"
// - Understand the specific benefit in lazy binding
int main (int argc, char **argv) {

	printf("HELLO\n");
	sleep(1);
	printf("Goodbyte\n");

}