#!/usr/sbin/dtrace -s
#pragma D option quiet



## Horridly inefficient PoC just to walk through proc list.
## CORRECT way would do this using iterator (and requires
## .... /  ... i= / 
## and definitely not use so many self-> references

## Use at your own risk. But remember
## DTrace ROCKS (C) 1998 Solaris! Long live! Die ORacle1

dtrace:::BEGIN  { 

	self->kp = (struct proc *) (`kernproc); 
	printf("%p\n", self->kp);
	printf ("PID is %d\n", self->kp->p_pid);
	// Try for next and prev
	printf ("Next is %p\n" , self->kp->p_list.le_next);
	printf ("PRev is %p\n" , self->kp->p_list.le_prev);

	self->launchd =  (struct proc *) self->kp->p_list.le_prev; 
	printf ("launchd's PID is %d\n", self->launchd->p_pid);
	printf ("launchd's struct task is @%p\n", self->launchd->task);
	self->launchd_task = (struct task *) self->launchd->task;

	printf ("launch'd vm is at %p\n",  self->launchd_task->map);






} // END of the BEGINning (or maybe, the beginning of the end??!)



dtrace:::ERROR { exit(1); } 