#include <mach/mach.h>
#include <mach/mach_port.h>
#include <bootstrap.h>  // for extern bootstrap_port
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv){

	printf ("0x%x\n", bootstrap_port);

	// A port 'name' is the uint32_t opaque identifier of a port
	// right in user mode.

	mach_port_name_t myPort;
	ipc_space_inspect_t target = mach_task_self();

	kern_return_t kr = mach_port_allocate(target, //  ipc_space_t task,
					MACH_PORT_RIGHT_RECEIVE, // mach_port_right_t right,
					&myPort); // mach_port_name_t *name


	printf("my port: 0x%x\n", myPort);

kern_return_t kr2 = mach_port_insert_right(target, // ( ipc_space_t task,
			myPort, // mach_port_name_t name,
			myPort, // mach_port_t poly,
			MACH_MSG_TYPE_MAKE_SEND); // mach_msg_type_name_t polyPoly

	printf("KR2 was: %d\n", kr2);





        ipc_info_space_t space_info;
        ipc_info_name_array_t table_info;
        mach_msg_type_number_t table_infoCnt;
        ipc_info_tree_name_array_t tree_info;
        mach_msg_type_number_t tree_infoCnt;


		kern_return_t kr1 =  mach_port_space_info (target, //  ipc_space_inspect_t task,
					&space_info, // ipc_info_space_t *space_info,
					&table_info, // ipc_info_name_array_t *table_info,
					&table_infoCnt, // mach_msg_type_number_t *table_infoCnt,
					// UNUSED
					&tree_info , // ipc_info_tree_name_array_t *tree_info,
					&tree_infoCnt); // mach_msg_type_number_t *tree_infoCnt

	if (kr1 != KERN_SUCCESS) {
	
	//	fprintf(stderr,"ERROR: %s\n", mach_error_string(mach_error));
	
		exit(1);
	}


#if 0
typedef struct ipc_info_name {
        mach_port_name_t iin_name;              /* port name, including gen number */
/*boolean_t*/ integer_t iin_collision;   /* collision at this entry? */
        mach_port_type_t iin_type;      /* straight port type */
        mach_port_urefs_t iin_urefs;    /* user-references */
        natural_t iin_object;           /* object pointer/identifier */
        natural_t iin_next;             /* marequest/next in free list */
        natural_t iin_hash;             /* hash index */
} ipc_info_name_t;
#endif
	int p = 0;
	for (p =0; p < table_infoCnt; p++) {
		printf("%d: 0x%x, Type: 0x%x, urefs: %d, Object: 0x%x\n",
			p,
			table_info[p].iin_name,
			table_info[p].iin_type,
			table_info[p].iin_urefs,
			table_info[p].iin_object);

	}

}