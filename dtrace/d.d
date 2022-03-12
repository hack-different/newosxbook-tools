#!/usr/bin/env dtrace -s
#pragma D option quiet

provider*:::  // Using "*" allows for matching per-pid providers
{
   // Convenience naming of probe variables
   method = (string)&probefunc[1];
   type = (string)&probefunc[0];
   class = probemod;
   name = probename;

   printf("%s(%d) %s %s %s %s ==> %x %x %x\n", 
	  execname, pid, method, type, name, class,
          arg1, arg2, arg3);
}