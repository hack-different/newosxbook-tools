#!/usr/sbin/dtrace -s
#pragma D option flowindent



syscall::open:entry { self->tracing = 1; }
syscall::open:return { self->tracing = 0; exit(0); }


fbt::: /self->tracing/ { printf("%x %x %x", arg0, arg1,arg2); /* Dump arguments */ }
