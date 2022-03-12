#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <net/if_utun.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>	// malloc
#include <arpa/inet.h> // inet_ntop

#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include "lsock.h"

#include <ncurses.h>

int wantCurses =1;
int wantUDP = 1;
int wantListen = 1;
int wantTCP = 1;
int wantColors = 1;
int wantRefresh = 1;

/**
  * lsock.c : A TCPView inspired netstat(1)-clone (in text mode) for OS X/iOS
  *
  * Coded by Jonathan Levin - but may be freely distributed (not to mention improved!)
  *
  * You want version 0.2, not this. (http://newosxbook.com/files/lsock.tar)
  *
  * Arguments: "nc" - disable full-screen (will automatically disable curses if piping output)
  *            "tcp" - start in TCP-only mode
  *            "udp" - start in UDP-only mode
  *
  * 
  * Commands for full screen mode: 'T' - TCP, 'U' - UDP, 'L' - Listeners, 'C' - Colors
  *
  * Possible improvements (which I may actually get to, if I have the time/space)
  *
  *    - GUI: Make this more like Windows' TCPView and Procmon
  *    - Remote monitoring
  *    - Periodic polling on sources: Currently only when a source notification is received do
  *      I poll its description
  *    - Add routing provider (prov #1)
  *    - Show provider statistics, too
  *
  * BUGS: When not filtering on TCP and/or UDP, sometimes not all sockets show. Need to fix that
  *       On iOS6, you need to kill UserEventAgent; some notifications don't make it.
  *       
  *       Note: To use curses on iOS, you will need to copy over /usr/share/terminfo from OS X.
  *             If you don't, ncurses will fail on initscr() - you will need "nc" instead
  *             
  *
  */

 

int fd;
void 
print_header (WantCurses)
{
	char *header = "Time     Local Addr        \tRemote Addr              \tIf\tState    \tPID   (Name)\n";
	if (WantCurses) {
	 
	attrset(COLOR_PAIR(0));
	 attrset(A_UNDERLINE| A_BOLD);
 	 mvwaddstr(stdscr, 1,0 ,header);
	attroff(A_UNDERLINE); attron(A_NORMAL);
	attrset(COLOR_PAIR(0));
	}
	else printf ("%s\n",  header);

}

int 
setupSocket(void)
{
        struct sockaddr_ctl sc;
        struct ctl_info ctlInfo;
        unsigned int num = 0;
        int fd;


        memset(&ctlInfo, 0, sizeof(ctlInfo));
        if (strlcpy(ctlInfo.ctl_name, "com.apple.network.statistics", sizeof(ctlInfo.ctl_name)) >=
            sizeof(ctlInfo.ctl_name)) {
                fprintf(stderr,"CONTROL NAME too long");
                return -1;
        }
        if ((fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL)) == -1) {
                fprintf(stderr,"socket(SYSPROTO_CONTROL): %s", strerror(errno));
                return -1;

        }
        if (ioctl(fd, CTLIOCGINFO, &ctlInfo) == -1) {
                fprintf(stderr,"ioctl(CTLIOCGINFO): %s", strerror(errno));
                close(fd);
                return -1;
        }
        sc.sc_id = ctlInfo.ctl_id;
        sc.sc_len = sizeof(sc);
        sc.sc_family = AF_SYSTEM;
        sc.ss_sysaddr = AF_SYS_CONTROL;
        sc.sc_unit = num + 1;           /* zero means unspecified */
        if (connect(fd, (struct sockaddr *)&sc, sizeof(sc)) == -1) {
                fprintf(stderr,"connect(AF_SYS_CONTROL): %s\n", strerror(errno));
		if (errno == EBUSY )
		{
		   printf("Apparently some other process is using the system socket. "
		   "On iOS, this is usually UserEventAgent. Kill it (-9!) and immediately start this program\n");
		}

                close(fd);
                return -1;
        }
         return fd;
}


const char *
stateToText(int State)
{

  switch(State)
	{
		case TCPS_CLOSED: return ("CLOSED");
		case TCPS_LISTEN: return ("LISTENING");
		case TCPS_ESTABLISHED: return ("ESTABLISHED");
		case TCPS_CLOSING: return ("CLOSING");
		case TCPS_SYN_SENT: return ("SYN_SENT");
		case TCPS_LAST_ACK: return ("LAST_ACK");
		case TCPS_CLOSE_WAIT: return ("CLOSE_WAIT");
		case TCPS_TIME_WAIT: return ("TIME_WAIT");
		case TCPS_FIN_WAIT_1 : return ("FIN_WAIT_1");
		case TCPS_FIN_WAIT_2 : return ("FIN_WAIT_2");

	
		default:
		  return("?");
	}

} // stateToText

#define MAX_DESC 2000
char *descriptors[MAX_DESC];
int descriptorsModded[MAX_DESC];

int maxDesc = 0;


char *message = "                                          ";

void 
setMessage (char *Message)
{
 	message = Message;
}
void 
print_descriptors()
{

	int i;
	int j = 3;


	if (!wantUDP && !wantTCP) { setMessage ( "Warning: Both TCP and UDP are filtered");}
	if (wantCurses) {
			attrset(COLOR_PAIR(0) | A_BOLD);
			mvwaddstr(stdscr, 2,0 ,message);

			attroff(A_BOLD);
			}

	if (wantCurses && wantRefresh)
	{
		wantRefresh = 0; erase();print_header(wantCurses);
	}

	for (i = 0; i < MAX_DESC; i++)
	{	
		if (descriptors[i])
		{
			// This is a quick and dirty example - So instead of mucking around with
			// descriptor data structures, I look at the text output of each, and figure
 			// out from it whether or not it's UDP, or the TCP state, etc..

			if (strstr(descriptors[i], "N/A")) // UDP have a state of "N/A"
			{
				if ( ! wantUDP) continue;
				if (wantColors) {
				  if (wantCurses) { init_pair(4,COLOR_CYAN, COLOR_BLACK); attrset(COLOR_PAIR(4)); }
				 else
				    printf (GREEN);
				
				}
			}
			else // TCP (not showing route for now)
			{
				if (!wantTCP) continue;
				
				if (wantColors)
					{ 
					if (wantCurses)	attrset(COLOR_PAIR(0)); else printf(NORMAL);
					}
		 		if (strstr(descriptors[i], "ESTABLISHED") )  {
					if (wantColors)
						{
					if (wantCurses) { init_pair(3, COLOR_GREEN, COLOR_BLACK); attrset(COLOR_PAIR(3));}
					else printf(GREEN); 
						}
					} // ESTABLISHED

		 		if (strstr(descriptors[i], "LISTEN") )  {
					if (!wantListen) continue;
					if (wantColors)
					  	{
					if (wantCurses) { init_pair(2, COLOR_BLUE, COLOR_BLACK); attrset(COLOR_PAIR(2));}
					else printf(BLUE); 
						}
					} // ESTABLISHED

		 		if (strstr(descriptors[i], "SYN_SENT") ) {
					if (wantColors) {
					if (wantCurses) { init_pair(5, COLOR_RED, COLOR_BLACK); attrset(COLOR_PAIR(5));}
					else printf(RED); 
						}
					} // SYN_SENT


			} // TCP
			if (wantCurses) {

 	 		mvwaddstr(stdscr, j++,0 ,descriptors[i]);
			}
			else
			{ if (descriptorsModded[i]) printf( "%s\n", descriptors[i]); }
			descriptorsModded[i] = 0;
		   
			if (wantColors && !wantCurses) printf (NORMAL);
		}

	}

	clrtobot();
	refresh();
	}

char *
print_udp_descriptor (char *desc, int prov, int num)
{

   nstat_udp_descriptor *nud = (nstat_udp_descriptor *) desc;

   
   int f;

  int ipv6 = 0;
  char localAddrPrint[40];
  char remoteAddrPrint[40];
  const char *localPrint, *remotePrint;
  unsigned short localPort, remotePort;
  char timeBuf[30];
  time_t	now;
  struct tm *n;
  char *returned = (char *) malloc(1024);

  returned[0]  ='\0'; // Dont need AF_ because we have this already in the address, right?
  time(&now);
   n = localtime(&now);
   
    memset(timeBuf,'\0', 30);
  strftime(timeBuf, 29, "%H:%M:%S", n);
 // sprintf(timeBuf, "%d......", num);

   sprintf (returned + strlen(returned), "%-8s ", timeBuf);
   if (nud->local.v4.sin_family == AF_INET6) {
		ipv6++; }
  if (!ipv6)
  {
     localPort = ntohs(nud->local.v4.sin_port);
     remotePort = ntohs(nud->remote.v4.sin_port);
  }
  else {
     localPort = ntohs(nud->local.v6.sin6_port);
     remotePort = ntohs(nud->remote.v6.sin6_port);
  }
  
  if (nud->remote.v6.sin6_family == AF_INET6) {
       localPrint = inet_ntop(nud->local.v6.sin6_family,&(nud->local.v6.sin6_addr), localAddrPrint, 40);
       remotePrint = inet_ntop(nud->remote.v6.sin6_family,&(nud->remote.v6.sin6_addr), remoteAddrPrint, 40);
  
 }
  if (nud->remote.v4.sin_family == AF_INET) {
       localPrint = inet_ntop(nud->local.v4.sin_family,&(nud->local.v4.sin_addr), localAddrPrint, 40);
       remotePrint = inet_ntop(nud->remote.v4.sin_family,&(nud->remote.v4.sin_addr), remoteAddrPrint, 40);
  
 }
  
  if (localAddrPrint) {
	 sprintf (localAddrPrint + strlen(localAddrPrint),":%-5hu", (unsigned short) localPort);
	if (remotePort == 0)
	{
        sprintf(returned + strlen(returned),"%-21s\t*.*                    \t        ", localAddrPrint);

	}
 	else
        sprintf(returned + strlen(returned), "%-21s\t%-21s %-5hu\t", localAddrPrint, remoteAddrPrint, (unsigned short) remotePort);
  
     }
   sprintf(returned + strlen(returned), "%d\tN/A         \t", nud->ifindex);
   sprintf(returned + strlen(returned), "%-5d ", nud->pid);
   if (nud->pname[0]) { sprintf(returned + strlen(returned), "(%s)",   nud->pname );}
   else { sprintf(returned + strlen(returned), "                 ");}
 
   return (returned);

}

void refreshSrc (int Prov, int Num)
{
	int rc ;
	nstat_msg_get_src_description gsdreq;

           gsdreq.hdr.type= 1005; //NSTAT_MSG_TYPE_GET_SRC_DESC
           gsdreq.srcref=Num;
           gsdreq.hdr.context = Prov; ;//;nmsa->provider;
           rc = write (fd, &gsdreq, sizeof(gsdreq));



}
char *
print_tcp_descriptor (char *desc, int prov, int num)
{

   nstat_tcp_descriptor *ntd = (nstat_tcp_descriptor *) desc;

   
  int ipv6 = 0;
  char localAddrPrint[40];
  char remoteAddrPrint[40];
  const char *localPrint, *remotePrint;
  unsigned short localPort, remotePort;
  char timeBuf[30];
  time_t	now;
  struct tm *n;

  char *returned = (char *) malloc(1024);

  returned[0]  ='\0'; // Dont need AF_ because we have this already in the address, right?
  time(&now);
   n = localtime(&now); 
  strftime(timeBuf, 29, "%H:%M:%S", n);

 //sprintf(timeBuf, "%d......", num);
   sprintf (returned + strlen(returned), "%-8s ", timeBuf);


// sprintf(returned+strlen(returned), "%d\t", ntd->sndbufused);

   if (ntd->local.v4.sin_family == AF_INET6) {
		//printf("IPv6\t"); 
		ipv6++; }
  if (!ipv6)
  {
     localPort = ntohs(ntd->local.v4.sin_port);
     remotePort = ntohs(ntd->remote.v4.sin_port);
  }
  else {
     localPort = ntohs(ntd->local.v6.sin6_port);
     remotePort = ntohs(ntd->remote.v6.sin6_port);
  }
  
  if (ntd->remote.v6.sin6_family == AF_INET6) {
       localPrint = inet_ntop(ntd->local.v6.sin6_family,&(ntd->local.v6.sin6_addr), localAddrPrint, 40);
       remotePrint = inet_ntop(ntd->remote.v6.sin6_family,&(ntd->remote.v6.sin6_addr), remoteAddrPrint, 40);
  
 }
  if (ntd->remote.v4.sin_family == AF_INET) {
       localPrint = inet_ntop(ntd->local.v4.sin_family,&(ntd->local.v4.sin_addr), localAddrPrint, 40);
       remotePrint = inet_ntop(ntd->remote.v4.sin_family,&(ntd->remote.v4.sin_addr), remoteAddrPrint, 40);
  
 }
  
  if (remoteAddrPrint)
	{
	   sprintf (remoteAddrPrint + strlen(remoteAddrPrint),":%-5hu", (unsigned short) remotePort);

	}
	else sprintf (remoteAddrPrint, "*.*");

  if (localAddrPrint) {
	 sprintf (localAddrPrint + strlen(localAddrPrint),":%-5hu", (unsigned short) localPort);
	if (remotePort == 0)
	{
        sprintf(returned + strlen(returned),"%-21s\t*.*                    \t        ", localAddrPrint);

	}
 	else
        sprintf(returned + strlen(returned), "%-21s\t%-25s\t", localAddrPrint,  remoteAddrPrint);
  
     }
   sprintf(returned + strlen(returned), "%d\t%-10s\t", ntd->ifindex,  stateToText(ntd->state));
   sprintf(returned + strlen(returned), "%-5d ", ntd->pid);
   if (ntd->pname[0]) { sprintf(returned + strlen(returned), "(%s)",   ntd->pname );}
   else { sprintf(returned + strlen(returned), "                 ");}
 
   if ( strstr(returned, "CLOSED") && strstr(returned,"("))
	{
		refreshSrc(prov,num);

	}
   return (returned);

}

int 
print_nstat_msg (void *msg, int size)
{
  nstat_msg_hdr *ns = (nstat_msg_hdr *) msg;
  
  switch (ns->type)
	{

	   case  10001: // NSTAT_MSG_TYPE_SRC_ADDED               = 10001
         {
	  	nstat_msg_src_added *nmsa = (nstat_msg_src_added *) (ns);
		
	     // printf("Src Added - %ld\n", sizeof(nstat_msg_src_added));
	//	printf ("PRovider: %d, SrcRef: %d\n", nmsa->provider, nmsa->srcref);
		
		return (10001);
		
		}
	      break;
	   case 10002: //NSTAT_MSG_TYPE_SRC_REMOVED             = 10002

		// remove this descriptor
	      return (10002);
		//remove_descriptor(...)

	    break;
	   case 10003: // ,NSTAT_MSG_TYPE_SRC_DESC                = 10003
	     {
	      nstat_msg_src_description *nmsd = (nstat_msg_src_description *) (ns );
		if (nmsd->provider == 2)
		descriptors[nmsd->srcref]  =print_tcp_descriptor((char *)nmsd->data, nmsd->provider,nmsd->srcref);
		else if (nmsd->provider == 3)
		 {descriptors[nmsd->srcref] = print_udp_descriptor((char *) nmsd->data, nmsd->provider, nmsd->srcref);
			}
  		descriptorsModded[nmsd->srcref] = 10;

		}
	      break;

           case 10004: //NSTAT_MSG_TYPE_SRC_COUNTS              = 10004
		{
		nstat_msg_src_counts *nmsc = (nstat_msg_src_counts *) (ns );
	//	printf ("RX: %d TX:%d\n", nmsc->counts.nstat_rxpackets , nmsc->counts.nstat_txpackets );

	
		} break;
	
	    case 1: 
		{
			if (ns->context < MAX_DESC && !descriptors[ns->context]) break;
			printf ("ERROR for context %lld - should be available\n", ns->context); break; 
		}
	    case 0: // printf("Success\n"); break;
		break;
	   default:
		printf("Type : %d\n", ns->type);
	}
	fflush(NULL);
return (ns->type) ;
}



int main (int argc, char **argv)
{

  
 int rc = 0;
 char c[2048];
 memset (&descriptors, '\0', sizeof(char *) * MAX_DESC);
 memset (&descriptorsModded, '\0', sizeof(int) * MAX_DESC);
  fd =setupSocket ();

 if (fd == -1)
	{
		fprintf(stderr,"Unable to continue without socket\n"); exit(1);
	}
 struct stat stbuf;
 fstat(1, &stbuf);
 
 if (stbuf.st_mode & S_IFCHR) { wantColors = 1; wantCurses = 1;} else {wantColors = 0; wantCurses = 0;}
 

 int arg = 1;
 for (arg = 1; arg < argc; arg++)
	{
 if (strcmp(argv[arg], "nc") == 0) wantCurses = 0;
 if (strcmp(argv[arg], "udp") == 0) { wantUDP = 1; wantTCP = 0;}
 if (strcmp(argv[arg], "tcp") == 0)  { wantTCP = 1; wantUDP = 0;}
	}



nstat_msg_query_src_req qsreq;

nstat_msg_add_all_srcs aasreq;

nstat_msg_get_src_description gsdreq;
char ch;



if (wantUDP)
{
aasreq.provider = 3; //
aasreq.hdr.type = 1002; //NSTAT_MSG_TYPE_ADD_ALL_SRCS
aasreq.hdr.context = 3;
rc = write (fd, &aasreq, sizeof(aasreq));
}

if (wantTCP)
{
aasreq.provider = 2 ;
aasreq.hdr.type = 1002; //NSTAT_MSG_TYPE_ADD_ALL_SRCS
aasreq.hdr.context = 2;
rc = write (fd, &aasreq, sizeof(aasreq));
}



 if (wantCurses)
	{
  initscr(); cbreak(); noecho(); start_color();
 nodelay (stdscr,1);

	}


	print_header(wantCurses);

while (1) { //rc > 0) {

  if (wantCurses)
	{
		fd_set	fds;
		struct timeval to;
		to.tv_sec = 0;
		to.tv_usec = 100;
		FD_ZERO (&fds);
		FD_SET (fd, &fds);
		// select on socket, rather than read..
		rc = select(fd +1, &fds, NULL, NULL, &to);

		if (rc > 0) rc = read(fd,c,2048);

	}
  else
     rc = read (fd, c, 2048);
  // check rc. Meh.


  if (rc > 0)
   {
     nstat_msg_hdr *ns = (nstat_msg_hdr *) c;

  switch (ns->type)
   {
	case 10001:
	  {
  	   nstat_msg_src_added *nmsa = (nstat_msg_src_added *) (c);

	   gsdreq.hdr.type= 1005; //NSTAT_MSG_TYPE_GET_SRC_DESC
	   gsdreq.srcref=nmsa->srcref;
	   gsdreq.hdr.context = 1005; // This way I can tell if errors get returned for dead sources
	   rc = write (fd, &gsdreq, sizeof(gsdreq));


  	   descriptorsModded[nmsa->srcref] = 1;
	   break;
	  }
	case 10002:
	  {
		//struct nstat_msg_src_removed *rem;


  	   nstat_msg_src_removed *nmsr = (nstat_msg_src_removed *) (c);
	   if(wantCurses) mvhline(nmsr->srcref + 2, 1, '-' , strlen(descriptors[nmsr->srcref]));
	   descriptors[nmsr->srcref] = NULL; // or free..
	   descriptorsModded [nmsr->srcref] = 1; // or free..
	   


	   
	   break;
	  }
	
	case 10003: 
 		 rc = print_nstat_msg(c,rc);
		break;

	case 10004:
		rc = print_nstat_msg(c,rc);
		break;
	case 0: 

	break;

	case 1:

	    //Error message - these are usually for dead sources (context will be 1005)
		break;
	
	default:

	break;
	
   }
  
	}

 if (wantCurses)
  {
	int i;
        ch = getch();
	if (ch != ERR) {
	if (ch == 'H' || ch =='h') printf("HELP\n");
	if (ch == 'U' || ch =='u') {wantUDP = !wantUDP; setMessage("Toggling UDP sockets"); }
	if (ch == 'T' || ch == 't') { wantTCP = !wantTCP; setMessage("Toggling TCP sockets"); }
	if (ch == 'L' || ch == 'l') { wantListen= !wantListen; setMessage("Toggling TCP listeners"); }


	if (ch == 'C' || ch == 'c') { wantColors = !wantColors; setMessage("Toggling color display"); }
	wantRefresh = 1;
	}
	

  }

  print_descriptors();

}


return (0);

}