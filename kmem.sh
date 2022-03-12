#!/bin/bash
#
## vars

# Main 
/usr/sbin/dtrace -n '
inline uint64_t a = '$1';
inline int      l = '$2';

#pragma D option quiet
dtrace:::BEGIN { tracemem(a,l); exit(0); }

dtrace:::ERROR { exit(1); } '