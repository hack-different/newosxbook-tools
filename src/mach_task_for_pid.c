#include <mach/mach.h> // Where all the good things are.
#include <mach/mach_port.h>
#include <stdio.h>
#include <bootstrap.h>
#include <unistd.h>
#include <stdlib.h> // because I can

extern mach_port_t	bootstrap_port;

int main (int argc, char **argv){

	if (argc < 2 ) { fprintf(stderr,"Usage: %s _name_\n", argv[0]); exit(1);}


	pid_t  myVictim = 0;
	kern_return_t kr = KERN_SUCCESS;

	printf("I just woke up in this dark room and all I see is - the BSP: 0x%x\n", bootstrap_port);

	int rc = sscanf(argv[1], "%d" , &myVictim);
	if (rc == 1) {	
		// myVictim holds a PID.

		}
	else {
	printf("I will now lookup %s\n", argv[1]);

	mach_port_t	thePort;
	 kr  = bootstrap_look_up(bootstrap_port, // bootstrap_port_t
					      argv[1], // service_name_t 
					     &thePort); // mach_port_t *

	if (KERN_SUCCESS == kr) {
		printf("Success!!! %s is at 0x%x\n", argv[1], thePort);

	}
	else printf("Nope\n");
	} // end lookup

	// The ports reside within a mach_task_t which is ITSELF a port.
	// A task (i.e. process) can access its own port (namely, get 
	// a send right to its representation in kernel) by calling
	// mach_task_self()

	printf ("I, myself, am : 0x%x\n", mach_task_self());

	ipc_info_space_t	space_info;
	ipc_info_name_array_t   table_info;
 	mach_msg_type_number_t  table_info_cnt;
	ipc_info_tree_name_array_t	tree_info;
	mach_msg_type_number_t		tree_info_cnt;
	mach_port_t victim_task_port = mach_task_self();
	
	if (myVictim) {
		if (getuid() != 0){
			fprintf(stderr, "Go away, little man! TFP is for r00t only\n");
	
			exit(1);
		}
			
		kr = task_for_pid (mach_task_self(), myVictim, &victim_task_port);

		}
	
	kr = mach_port_space_info ( victim_task_port, // ipc_space_t 
				    &space_info,      // ipc_space_info_t
				    &table_info,     // ipc_info_name_array_t
				    &table_info_cnt, // ...
				    &tree_info,
				    &tree_info_cnt);

	if (kr == KERN_SUCCESS) {
		int p = 0;
		for (p = 0; p <  table_info_cnt; p++) {
			printf("Port 0x%x (obj 0x%x) is of type: 0x%x\n",
				table_info[p].iin_name,  table_info[p].iin_object, table_info[p].iin_type);
			}



	}

	sleep(60);


}

