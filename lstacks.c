#include <mach/mach.h>
#include <stdio.h>
#include <stdlib.h>
#include <libproc.h>

//#include <mach/thread_info.h>
//#include <mach/task_info.h>

int main(int argc, char **argv)
{
  
  host_t	myhost = mach_host_self();

  host_t	host_priv;


  mach_port_t	psDefault;
  mach_port_t	psDefault_control;

  task_array_t	tasks; 
  mach_msg_type_number_t numTasks;
  int i;

   thread_array_t 	threads;
   thread_info_data_t	tInfo;

  kern_return_t kr;

 //hook("/usr/lib/libSystem.B.dylib", "mach_msg", mach_msg_hook);
  host_get_host_priv_port(mach_host_self(), &host_priv);
  kr = processor_set_default(host_priv, &psDefault);

  processor_set_name_array_t	*psets = malloc(1024);
  mach_msg_type_number_t  	psetCount;

 kr = host_processor_sets (
        host_priv, // host_priv_t host_priv,
        psets, //  processor_set_name_array_t *processor_sets,
         &psetCount); // mach_msg_type_number_t *processor_setsCnt

  printf("COUNT: %d\n", psetCount);
  kr = host_processor_set_priv(host_priv, psDefault, &psDefault_control); 
  if (kr != KERN_SUCCESS) { fprintf(stderr, "host_processor_set_priv failed with error %x\n", kr);  mach_error("host_processor_set_priv",kr); exit(1);}

  printf("So far so good\n");

  numTasks=1000;
  kr = processor_set_tasks(psDefault_control, &tasks, &numTasks); 
  if (kr != KERN_SUCCESS) { fprintf(stderr,"processor_set_tasks failed with error %x\n",kr); exit(1); }

  for (i = 0; i < numTasks; i++)
	{
		char name[128];
	

		int pid;
 		pid_for_task(tasks[i], &pid);
		int rc=  proc_name(pid, name, 128);
		printf("PID: %d %s\n",pid,name);
	}

   return (MACH_PORT_NULL);
}
