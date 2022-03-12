#!/bin/bash
#
## vars

# Main 
/usr/sbin/dtrace -n '
inline int z = '$1';
#pragma D option quiet


dtrace:::BEGIN  { printf("%s\n", stringof((`zone_array)[z].zone_name));  
		tracemem ((void *) (&(`zone_array)[z]), sizeof(struct zone)); }


dtrace:::ERROR { exit(1); } '