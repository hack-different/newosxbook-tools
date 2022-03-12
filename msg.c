#include <mach/mach.h>
#include <mach/mach_port.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char **argv) {
	// Sample for mach messaging:
	//
	// demonstrating using mach_msg to send and possibly receive a message
	//

	kern_return_t kr;

	mach_port_t	portA, portB = MACH_PORT_NULL;

	kr = mach_port_allocate(mach_task_self(),
				 MACH_PORT_RIGHT_RECEIVE,
				&portA);

	kr = mach_port_allocate(mach_task_self(),
				 MACH_PORT_RIGHT_RECEIVE,
				&portB);
	
	printf("Sending message from port A (0x%x) to port B (0x%x)\n", portA, portB);

	mach_msg_header_t *msg; 
	size_t	bufSize	= 128;
	char *buf = alloca(bufSize);
	memset (buf, 'J', 128);


#if 0
	// Should insert SEND right.. (optional, if we use MIG style "19":
	kr = mach_port_insert_right (mach_task_self(), //...
				     portB, // remote port I will send to
				     portB,
				 MACH_MSG_TYPE_MAKE_SEND ); // new right name (on old right)

#endif
	printf("KR: %x\n", kr);
	///
	msg = (mach_msg_header_t *) buf;

	/////////////////
	msg->msgh_id = 0x24242424;
	msg->msgh_remote_port = portB;
	msg->msgh_local_port = MACH_PORT_NULL;
	msg->msgh_size = 128;
	msg->msgh_voucher_port = MACH_PORT_NULL;
	msg->msgh_reserved = 0;
	// "19" is COPY_SEND - which is what MIG would do. BUT WE NEED TO ENSURE WE HAVE A SEND!
	// Use insert_right, above, OR... easier, instruct kernel to MAKE_SEND on the fly.

	msg->msgh_bits =  MACH_MSGH_BITS( MACH_MSG_TYPE_MAKE_SEND, 0) ;; 
	

	// 

	mach_msg_return_t   mmr =        mach_msg( msg, // mach_msg_header_t *msg,
					MACH_SEND_MSG,  // | MACH_RCV_MSG   -- mach_msg_option_t option,
					128, // mach_msg_size_t send_size, 
					0, // mach_msg_size_t rcv_size,
					MACH_PORT_NULL, // mach_port_name_t rcv_name,
					MACH_MSG_TIMEOUT_NONE, // mach_msg_timeout_t timeout, 
					MACH_PORT_NULL); // mach_port_name_t notify);

		printf("MMR: 0x%x\n", mmr);
		if (mmr == MACH_MSG_SUCCESS) { sleep(10);}






}