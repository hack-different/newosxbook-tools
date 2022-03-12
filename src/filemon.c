
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>     // for _IOW, a macro required by FSEVENTS_CLONE
#include <sys/types.h>     // for uint32_t and friends, on which fsevents.h relies
#include <unistd.h>
#include <string.h> // memset
//#include <sys/_types.h>     // for uint32_t and friends, on which fsevents.h relies

#include <sys/sysctl.h> // for sysctl, KERN_PROC, etc.
#include <errno.h>

//#include <sys/fsevents.h>

#include "fsevents.h"
#include "colors.h" 
#include <signal.h> // for kill



/**
 *
 * Filemon: A simple but useful fsevents client utilty for OS X and iOS 
 *
 * Author: Jonathan Levin, TWTR: @Morpheus______ http://NewOSXBook.com/
 *
 * For details, q.v. MOXiI 1st Ed, chapter 3 (pp 74-78), or MOXiI II, Volume I, Chapter 5
 *
 * Source is wide open, so you are free to use and abuse, but leaving credit to the original
 * author and the book website (http://NewOSXBook.com/) would be appreciated.
 *
 * New in 2.0:
 *   - Supports filters: Process IDs, names or events
 *   - Supports auto-stop and auto-link
 *   - Color
 * 
 */

int g_dumpArgs =0;

#define BUFSIZE 256 *1024

#define COLOR_OP YELLOW
#define COLOR_PROC BLUE
#define COLOR_PATH CYAN

// Utility functions
static char *
typeToString (uint32_t	Type)
{
	switch (Type)
	{
		case FSE_CREATE_FILE: return      ("Created       ");
		case FSE_DELETE: return           ("Deleted       ");
		case FSE_STAT_CHANGED: return     ("Changed stat  ");
		case FSE_RENAME:       return     ("Renamed       ");
		case FSE_CONTENT_MODIFIED: return ("Modified      ");
		case FSE_CREATE_DIR:	return    ("Created dir   ");
		case FSE_CHOWN:	        return    ("Chowned       ");

		case FSE_EXCHANGE: return            ("Exchanged     "); /* 5 */
		case FSE_FINDER_INFO_CHANGED: return ("Finder Info   "); /* 6 */
		case FSE_XATTR_MODIFIED: return      ("Changed xattr "); /* 9 */
	 	case FSE_XATTR_REMOVED: return       ("Removed xattr "); /* 10 */

		case FSE_DOCID_CREATED: return ("DocID created "); // 11
		case FSE_DOCID_CHANGED: return ("DocID changed "); // 12

		default : return ("?! ");

	}
}

int lastPID = 0;
static char *
getProcName(long pid)
{

  static char procName[4096];
  size_t len = 1000;
  int rc;
  int mib[4];

  // minor optimization
  if (pid != lastPID) 
  {
  memset(procName, '\0', 4096);

        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_PID;
        mib[3] = pid;

        if ((rc = sysctl(mib, 4, procName, &len, NULL,0)) < 0)
                {
                perror("trace facility failure, KERN_PROC_PID\n");
                exit(1);
                }

	//printf ("GOT PID: %d and rc: %d -  %s\n", mib[3], rc, ((struct kinfo_proc *)procName)->kp_proc.p_comm);

	lastPID = pid;
   }
         return (((struct kinfo_proc *)procName)->kp_proc.p_comm);


}

int 
doArg(char *arg, int Print)
{
	// Dump an arg value
	unsigned short *argType = (unsigned short *) arg;
	unsigned short *argLen   = (unsigned short *) (arg + 2);
	uint32_t	*argVal = (uint32_t *) (arg+4);
	uint64_t	*argVal64 = (uint64_t *) (arg+4);
	dev_t		*dev;
	char		*str;



	switch (*argType)
		{

		case FSE_ARG_INT64: // This is a timestamp field on the FSEvent
			if (g_dumpArgs) printf ("Arg64: %lld ", *argVal64);
			break;

		case FSE_ARG_STRING:
		 	str = (char *)argVal;
			if (Print) printf("%s ", str);
			break;
			
		case FSE_ARG_DEV:
			dev = (dev_t *) argVal;
			if (g_dumpArgs) printf ("DEV: %d,%d ", major(*dev), minor(*dev)); break;

		case FSE_ARG_MODE:
			if (g_dumpArgs) printf("MODE: %x ", *argVal); break;
		case FSE_ARG_PATH:
			printf ("PATH: " ); break;
		case FSE_ARG_INO:
			if (g_dumpArgs) printf ("INODE: %d ", *argVal); break;
		case FSE_ARG_UID:
			if (g_dumpArgs) printf ("UID: %d ", *argVal); break;
		case FSE_ARG_GID:
			if (g_dumpArgs) printf ("GID: %d ", *argVal); break;
#define FSE_ARG_FINFO    0x000c   // next arg is a packed finfo (dev, ino, mode, uid, gid)
		case FSE_ARG_FINFO:
			printf ("FINFO\n"); break;
		case FSE_ARG_DONE:	if (Print)printf("\n");return 2;

		default:
			printf ("(ARG of type %hd, len %hd)\n", *argType, *argLen);
			exit(0);


		}

	return (4 + *argLen);

}


#define COLOR_OPTION	"-c"
#define COLOR_LONG_OPTION	"--color"

#define FILTER_PROC_OPTION	"-p"
#define FILTER_PROC_LONG_OPTION	"--proc"
#define FILTER_FILE_OPTION	"-f"
#define FILTER_FILE_LONG_OPTION	"--file"
#define FILTER_EVENT_OPTION	"-e"
#define FILTER_EVENT_LONG_OPTION "--event"
#define STOP_OPTION	"-s"
#define STOP_LONG_OPTION	"--stop"
#define LINK_OPTION	"-l"
#define LINK_LONG_OPTION	"--link"

void
usage()
{

	fprintf(stderr,"Usage: %s [options]\n", getprogname());
	fprintf(stderr,"Where [options] are optional, and may be any of:\n");
	fprintf(stderr,"\t" FILTER_PROC_OPTION  "|" FILTER_PROC_LONG_OPTION  "  pid/procname:  filter only this process or PID\n");
	fprintf(stderr,"\t" FILTER_FILE_OPTION  "|" FILTER_FILE_LONG_OPTION  "  string[,string]:        filter only paths containing this string (/ will catch everything)\n");
	fprintf(stderr,"\t" FILTER_EVENT_OPTION "|" FILTER_EVENT_LONG_OPTION " event[,event]: filter only these events\n");
	fprintf(stderr,"\t" STOP_OPTION "|" STOP_LONG_OPTION         ":                auto-stop the process generating event\n");
	fprintf(stderr,"\t" LINK_OPTION "|" LINK_LONG_OPTION         ":                auto-create a hard link to file (prevents deletion by program :-)\n");
	fprintf(stderr,"\t" COLOR_OPTION "|" COLOR_LONG_OPTION " (or set JCOLOR=1 first)\n");



}
// And.. Ze Main


#define MAX_FILTERS	10

int 
interesting_process (int pid, char *Filters[], int NumFilters)
{
	if (!NumFilters) return 1; 
	return 0;
}
	
int
interesting_file (char *FileName, char *Filters[], int NumFilters)
{

	// if no filters - everything is interesting
	if (!NumFilters) return 1; 

	

	
	while (NumFilters > 0)
	{
// fprintf(stderr,"Checking %s vs %s\n", FileName, Filters[NumFilters-1]);
	   if (strstr(FileName, Filters[NumFilters-1])) return 1;
	   NumFilters--;

	}

	return 0;
}

int
main (int argc, char **argv)
{
	int fsed, cloned_fsed;
	int i; 
	int rc;
	fsevent_clone_args  clone_args;
        unsigned short *arg_type;
	char buf[BUFSIZE];
	int autostop = 0;
	int autolink = 0;
	char *fileFilters[MAX_FILTERS]= { 0 };
	char *procFilters[MAX_FILTERS]= { 0 };

	int numFileFilters = 0;
	int numProcFilters = 0;
	int color = 0;

	if (getenv("JCOLOR")) color++;
	//if (argc > 1) { if (strcmp(argv[1],"-v") == 0) g_dumpArgs++; }

	int arg = 1;

	for (arg = 1; arg < argc; arg++)
	{
	   if (strcmp(argv[arg], "-h") == 0) { usage(); exit(1);}

	   if ((strcmp(argv[arg], FILTER_PROC_OPTION) == 0) ||
	       (strcmp(argv[arg], FILTER_PROC_LONG_OPTION) == 0))
		{
			if (arg == argc -1)
			{
			   fprintf(stderr, "%s: Option requires an argument\n",
				   argv[arg]); 
			   exit(2);
			}
			numProcFilters++;

			arg++; continue;
		}


	   if ((strcmp(argv[arg], FILTER_EVENT_OPTION) == 0) ||
	       (strcmp(argv[arg], FILTER_EVENT_LONG_OPTION) == 0))
		{
			if (arg == argc -1)
			{
			   fprintf(stderr, "%s: Option requires an argument\n",
				   argv[arg]); 
			   exit(2);
			}

			arg++; continue;
		}

	   if ((strcmp(argv[arg], FILTER_FILE_OPTION) == 0) ||
	       (strcmp(argv[arg], FILTER_FILE_LONG_OPTION) == 0))
		{
			if (arg == argc -1)
			{
			   fprintf(stderr, "%s: Option requires an argument\n",
				   argv[arg]); 
			   exit(2);
			}

			// Got it - add filters, separate by ","

			char *begin = argv[arg+1];
			char *sep = strchr (begin, ',');
			while (sep)
			{
			      *sep = '\0';
			      fprintf(stderr,"Adding File filter %d: %s\n", numFileFilters, begin);
			      fileFilters[numFileFilters++] = strdup(begin);
			      begin = sep + 1;
			      sep = strchr (begin, ',');
			   
			}
			fprintf(stderr,"Adding File filter %d: %s\n", numFileFilters, begin);
		        fileFilters[numFileFilters++] = strdup(begin);
			arg++; continue;
		}


	   if ((strcmp(argv[arg], COLOR_OPTION) == 0) || (strcmp(argv[arg], COLOR_LONG_OPTION) == 0))
		{

			color++;
			continue;
		}
	   if ((strcmp(argv[arg], STOP_OPTION) == 0) || (strcmp(argv[arg], STOP_LONG_OPTION) == 0))
		{
			autostop++;
			continue;
		}

	   if ((strcmp(argv[arg], LINK_OPTION) == 0) || (strcmp(argv[arg], LINK_LONG_OPTION) == 0))
		{
			autolink++;
			 continue;
		}

	   fprintf(stderr, "%s: Unknown option\n", argv[arg]); exit(3);
	}

	// This is for your own good: If autostop is allowed on everything, there is a chance a kill -STOP will be 
	// sent to your own terminal app or SSH.

	if (autostop && (!numFileFilters && !numProcFilters))
	{
		fprintf(stderr,"Error: Cannot allow auto-stopping of processes without either a file or process filter.\nIf you are sure you want to do this, set a null filter\n"); exit(4);
	}
	// Open the device
	fsed = open ("/dev/fsevents", O_RDONLY);

	int8_t	events[FSE_MAX_EVENTS];

	if (geteuid())
	{
		fprintf(stderr,"Opening /dev/fsevents requires root permissions\n");
	}

	if (fsed < 0)
	{
		perror ("open");
		 exit(1);
	}


	// Prepare event mask list. In our simple example, we want everything
	// (i.e. all events, so we say "FSE_REPORT" all). Otherwise, we 
	// would have to specifically toggle FSE_IGNORE for each:
	//
	// e.g. 
	//       events[FSE_XATTR_MODIFIED] = FSE_IGNORE;
	//       events[FSE_XATTR_REMOVED]  = FSE_IGNORE;
	// etc..
	for (i = 0; i < FSE_MAX_EVENTS; i++)
	{
		events[i] = FSE_REPORT; 
	}

	// But in v2.0, we allow user to specify events

	// Get ready to clone the descriptor:

	memset(&clone_args, '\0', sizeof(clone_args));
	clone_args.fd = &cloned_fsed; // This is the descriptor we get back
	clone_args.event_queue_depth = 100; // Makes all the difference
	clone_args.event_list = events;
	clone_args.num_events = FSE_MAX_EVENTS;
	
	// Do it.

	rc = ioctl (fsed, FSEVENTS_CLONE, &clone_args);
	
	if (rc < 0) { perror ("ioctl"); exit(2);}
	
	// We no longer need original..

	close (fsed);

	
	// And now we simply read, ad infinitum (aut nauseam)

	while ((rc = read (cloned_fsed, buf, BUFSIZE)) > 0)
	{
		// rc returns the count of bytes for one or more events:
		int offInBuf = 0;

		while (offInBuf < rc) {
	
		   // printf("----%d/%d bytes\n",offInBuf,rc);
	
		   struct kfs_event_a *fse = (struct kfs_event_a *)(buf + offInBuf);
		   struct kfs_event_arg *fse_arg;


	//		if (offInBuf) { printf ("Next event: %d\n", offInBuf);};

			if (fse->type == FSE_EVENTS_DROPPED)
			{
				printf("Some events dropped\n");
				break;
			}

		   if (!fse->pid){ printf ("%x %x\n", fse->type, fse->refcount); }

		   int print = 0;
		   char *procName = getProcName(fse->pid);
		   offInBuf+= sizeof(struct kfs_event_a);
		   fse_arg = (struct kfs_event_arg *) &buf[offInBuf];

		   if (interesting_process(fse->pid, procFilters, numProcFilters) 
                    && interesting_file (fse_arg->data, fileFilters, numFileFilters))
		   {
		   	printf ("%5d %s%s%s\t%s%s%s ", fse->pid, 
				color ? COLOR_PROC: "" , procName,  color ? NORMAL : "",
				color ? COLOR_OP : "", typeToString(fse->type), color ? NORMAL : "" );

		        // The path name is null terminated, so that's cool
		        printf ("%s%s%s\t", 
				color ? COLOR_PATH : "" , fse_arg->data, color ? NORMAL :"");
		     
  			if (fse->type == FSE_CREATE_FILE && autolink && (fse->pid != getpid()))
			{
			   int fileLen = strlen(fse_arg->data);
			   char *linkName = malloc (fileLen + 20);
			   strcpy(linkName, fse_arg->data);
			   snprintf(linkName + fileLen, fileLen + 20, ".filemon.%d", autolink);
			   int rc = link (fse_arg->data, linkName);
			   if (rc) { fprintf(stderr,"%sWarning: Unable to autolink %s%s - file must have been deleted already\n", 
					color ? RED : "", 
					fse_arg->data,
					color ? NORMAL : "");}

			   else    { fprintf(stderr,"%sAuto-linked %s to %s%s\n", 
					color ? GREEN : "", 
					fse_arg->data, linkName,
					color ? NORMAL :"");
			   		autolink++;
				   }

			   free (linkName);
			}

		   // Autostop only if this is a file creation, and interesting
		   if (autostop && fse->type == FSE_CREATE_FILE ) { 
				  fprintf(stderr, "%sAuto-stopping process %s (%d) on file operation%s\n", 
						color ? RED : "",
						    procName,
						  fse->pid,
						color ? NORMAL : "");
			           kill (fse->pid, SIGSTOP);
				 }

			print = 1;


		   }

	           offInBuf += sizeof(kfs_event_arg) + fse_arg->pathlen ;

		   int arg_len = doArg(buf + offInBuf,print);
	           offInBuf += arg_len;
		   while (arg_len >2 && offInBuf < rc)
			{
		   	    arg_len = doArg(buf + offInBuf, print);
	           	    offInBuf += arg_len;
			}
	
		}
		   memset (buf,'\0', BUFSIZE);
		if (rc > offInBuf) { printf ("*** Warning: Some events may be lost\n"); }
	}

} // end main