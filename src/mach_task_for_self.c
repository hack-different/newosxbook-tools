#include <mach/mach.h>
#include <mach/mach_port.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>

int main (int argc, char **argv) {

	mach_port_t myPort;
	 mach_port_t target = mach_task_self();
	kern_return_t kr;

	if (argc > 1) {
	 uint32_t pid = atoi(argv[1]);
			kr = task_for_pid(mach_task_self(),
					pid,
				&target);
			if (kr !=0) {
			fprintf(stderr,"No such luck my friend!\n"); exit(1);}

		}
	 kr = mach_port_allocate 
		(target, // ipc_space_t, 
		  MACH_PORT_RIGHT_RECEIVE,  // from port.h
		    &myPort); // mach_port *

	if (kr != KERN_SUCCESS) { exit(1);}

	 printf("My New Port is 0x%x\n",myPort);
	// port identifier returned is a mix of
	// an index and a generational number
	// e..g 0x707  e.g. 0x1203

	// step 2: Enumerating target's port space:
        ipc_info_space_t space_info;
        ipc_info_name_array_t table_info;
        mach_msg_type_number_t  table_infoCnt = sizeof(ipc_info_name_array_t);
        ipc_info_tree_name_array_t tree_info;
        mach_msg_type_number_t tree_infoCnt;

	// NOTE: Normally Cnt ("counts") are initialized to sizeof(...)
	// upon entry. And - if by ref - will be actual sizeof (in case
	// of variable length) on output.

	kr = mach_port_space_info ( target, // ipc_space_inspect_t task,
			&space_info, // ipc_info_space_t *space_info,
			&table_info, // ipc_info_name_array_t *table_info,
			&table_infoCnt, // mach_msg_type_number_t *table_infoCnt,
			&tree_info, // ipc_info_tree_name_array_t *tree_info,
			&tree_infoCnt); // mach_msg_type_number_t *tree_infoCnt

	// Tree_info : deprecated (not even set) as of 10.7-8(?)
	// we can only care about table and  space, and - for this humble sample
	// only table
#if 0
	iin_name
/*boolean_t*/ integer_t iin_collision;   /* collision at this entry? */
        mach_port_type_t iin_type;      /* straight port type */
        mach_port_urefs_t iin_urefs;    /* user-references */
        natural_t iin_object;           /* object pointer/identifier */
        natural_t iin_next;             /* marequest/next in free list */
        natural_t iin_hash;             /* hash index */
} ipc_info_name_t;


#endif

	int port_index= 0;
	for (port_index = 0; 
		port_index <  table_infoCnt;
		port_index++) {
		printf("Name @%d: is 0x%x (object: 0x%x), Type: 0x%x, urefs: %d\n",
			port_index,
			table_info[port_index].iin_name,
			table_info[port_index].iin_object,
			table_info[port_index].iin_type,
			table_info[port_index].iin_urefs);
	


	}; 


	sleep(200);
	




}
