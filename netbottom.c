#include <dispatch/dispatch.h>

#include <CoreFoundation/CoreFoundation.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


/**
 * Netbottom: A nettop(1) clone without the curses that are curses.
 * ----------
 *
 * Jonathan Levin, http://NewOSXBook.com/
 *
 * This is a simple program meant to accompany the bonus networking chapter I'm putting
 * into MOXiI 2 Volume I (read http://NewOSXBook.com/ChangeLog.html for the story behind that)
 *
 * This is essentially doing what my old lsock(j) did, but I no longer maintain lsock because
 * A) AAPL keeps breaking the APIs with every single version and B) some people have closed source
 * my sample and actually started selling it! 
 *
 * The advantage in this example is that it's using the private NetworkStatistics.framework, so that
 * hides all the underlying com.apple.network.statistics #%$@#% that keeps breaking on me.
 *
 * 
 * Variables:
 *
 *     JCOLOR = 0 to disable color (which is on by default)
 *     RESOLV = 1 to add DNS resolving (may be slower)
 *
 * This compiles on both MacOS and iOS cleanly:
 *  To compile for MacOS:
 *     gcc netbottom.c -o /tmp/netbottom.x86 -framework CoreFoundation -framework NetworkStatistics   -framework CoreFoundation -F /System/Library/PrivateFrameworks
 *
 * 
 * To compile on *OS you'll first need to create the "tbd" file for NetworkStatistics, since it's a private framework and there
 * are no TBDs for it. You'll need to first use jtool's --tbd option (in Alpha 4 and later) on the dyld_shared_cache_arm64, like so:
 *
 *
 * mkdir -p /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/System/Library/PrivateFrameworks/NetworkStatistics.framework/
 *
 * jtool2 --tbd /Volumes/PeaceB16B92.N102OS/System/Library/Caches/com.apple.dyld/dyld_shared_cache_arm64:NetworkStatistics \
 *     > /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/System/Library/PrivateFrameworks/NetworkStatistics.framework/NetworkStatistics.tbd
 *
 * and then,
 *
 *     gcc-arm64  netbottom.c -o /tmp/netbottom.x86 -framework CoreFoundation -framework NetworkStatistics   -framework CoreFoundation -F /System/Library/PrivateFrameworks
 *
 * On iOS you'll also need an entitlement 

<pre>
#if 0
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
        <key>com.apple.private.network.statistics</key>
        <true/>
        <key>platform-application</key>
        <true/>
</dict>
</plist>
#endif
</pre>

 * and/or sysctl net.statistics_privcheck=0
 *


 *
 *
 * PoC ONLY. There is a LOT that can be fixed here - for one, DNS lookups in main queue may block for a long while!
 *           also because I don't listen on events I don't get all the TCP state changes.
 *
 * License: FREE, ABSE, But again - if you plan on close sourcing this, it would be nice if you A) let me know and B) give credit
 * where due (That means AAPL, for awesome (though Block-infested) APIs, and this sample.
 *
 */



// The missing NetworkStatistics.h...
typedef void 	*NStatManagerRef;
typedef void 	*NStatSourceRef;

NStatManagerRef	NStatManagerCreate (const struct __CFAllocator *,
			     dispatch_queue_t,
			     void (^)(void *, void *));

int NStatManagerSetInterfaceTraceFD(NStatManagerRef, int fd);
int NStatManagerSetFlags(NStatManagerRef, int Flags);
int NStatManagerAddAllTCPWithFilter(NStatManagerRef, int something, int somethingElse);
int NStatManagerAddAllUDPWithFilter(NStatManagerRef, int something, int somethingElse);
void *NStatSourceQueryDescription(NStatSourceRef);

extern CFStringRef kNStatProviderInterface;
extern CFStringRef kNStatProviderRoute;
extern CFStringRef kNStatProviderSysinfo;
extern CFStringRef kNStatProviderTCP;
extern CFStringRef kNStatProviderUDP;
extern CFStringRef kNStatSrcKeyAvgRTT;
extern CFStringRef kNStatSrcKeyChannelArchitecture;
extern CFStringRef kNStatSrcKeyConnProbeFailed;
extern CFStringRef kNStatSrcKeyConnectAttempt;
extern CFStringRef kNStatSrcKeyConnectSuccess;
extern CFStringRef kNStatSrcKeyDurationAbsoluteTime;
extern CFStringRef kNStatSrcKeyEPID;
extern CFStringRef kNStatSrcKeyEUPID;
extern CFStringRef kNStatSrcKeyEUUID;
extern CFStringRef kNStatSrcKeyInterface;
extern CFStringRef kNStatSrcKeyInterfaceCellConfigBackoffTime;
extern CFStringRef kNStatSrcKeyInterfaceCellConfigInactivityTime;
extern CFStringRef kNStatSrcKeyInterfaceCellUlAvgQueueSize;
extern CFStringRef kNStatSrcKeyInterfaceCellUlMaxQueueSize;
extern CFStringRef kNStatSrcKeyInterfaceCellUlMinQueueSize;
extern CFStringRef kNStatSrcKeyInterfaceDescription;
extern CFStringRef kNStatSrcKeyInterfaceDlCurrentBandwidth;
extern CFStringRef kNStatSrcKeyInterfaceDlMaxBandwidth;
extern CFStringRef kNStatSrcKeyInterfaceIsAWD;
extern CFStringRef kNStatSrcKeyInterfaceIsAWDL;
extern CFStringRef kNStatSrcKeyInterfaceIsCellFallback;
extern CFStringRef kNStatSrcKeyInterfaceIsExpensive;
extern CFStringRef kNStatSrcKeyInterfaceLinkQualityMetric;
extern CFStringRef kNStatSrcKeyInterfaceName;
extern CFStringRef kNStatSrcKeyInterfaceThreshold;
extern CFStringRef kNStatSrcKeyInterfaceType;
extern CFStringRef kNStatSrcKeyInterfaceTypeCellular;
extern CFStringRef kNStatSrcKeyInterfaceTypeLoopback;
extern CFStringRef kNStatSrcKeyInterfaceTypeUnknown;
extern CFStringRef kNStatSrcKeyInterfaceTypeWiFi;
extern CFStringRef kNStatSrcKeyInterfaceTypeWired;
extern CFStringRef kNStatSrcKeyInterfaceUlBytesLost;
extern CFStringRef kNStatSrcKeyInterfaceUlCurrentBandwidth;
extern CFStringRef kNStatSrcKeyInterfaceUlEffectiveLatency;
extern CFStringRef kNStatSrcKeyInterfaceUlMaxBandwidth;
extern CFStringRef kNStatSrcKeyInterfaceUlMaxLatency;
extern CFStringRef kNStatSrcKeyInterfaceUlMinLatency;
extern CFStringRef kNStatSrcKeyInterfaceUlReTxtLevel;
extern CFStringRef kNStatSrcKeyInterfaceWifiConfigFrequency;
extern CFStringRef kNStatSrcKeyInterfaceWifiConfigMulticastRate;
extern CFStringRef kNStatSrcKeyInterfaceWifiDlEffectiveLatency;
extern CFStringRef kNStatSrcKeyInterfaceWifiDlErrorRate;
extern CFStringRef kNStatSrcKeyInterfaceWifiDlMaxLatency;
extern CFStringRef kNStatSrcKeyInterfaceWifiDlMinLatency;
extern CFStringRef kNStatSrcKeyInterfaceWifiScanCount;
extern CFStringRef kNStatSrcKeyInterfaceWifiScanDuration;
extern CFStringRef kNStatSrcKeyInterfaceWifiUlErrorRate;
extern CFStringRef kNStatSrcKeyLocal;
extern CFStringRef kNStatSrcKeyMinRTT;
extern CFStringRef kNStatSrcKeyPID;
extern CFStringRef kNStatSrcKeyProbeActivated;
extern CFStringRef kNStatSrcKeyProcessName;
extern CFStringRef kNStatSrcKeyProvider;
extern CFStringRef kNStatSrcKeyRcvBufSize;
extern CFStringRef kNStatSrcKeyRcvBufUsed;
extern CFStringRef kNStatSrcKeyReadProbeFailed;
extern CFStringRef kNStatSrcKeyRemote;
extern CFStringRef kNStatSrcKeyRouteDestination;
extern CFStringRef kNStatSrcKeyRouteFlags;
extern CFStringRef kNStatSrcKeyRouteGateway;
extern CFStringRef kNStatSrcKeyRouteGatewayID;
extern CFStringRef kNStatSrcKeyRouteID;
extern CFStringRef kNStatSrcKeyRouteMask;
extern CFStringRef kNStatSrcKeyRouteParentID;
extern CFStringRef kNStatSrcKeyRxBytes;
extern CFStringRef kNStatSrcKeyRxCellularBytes;
extern CFStringRef kNStatSrcKeyRxDupeBytes;
extern CFStringRef kNStatSrcKeyRxOOOBytes;
extern CFStringRef kNStatSrcKeyRxPackets;
extern CFStringRef kNStatSrcKeyRxWiFiBytes;
extern CFStringRef kNStatSrcKeyRxWiredBytes;
extern CFStringRef kNStatSrcKeySndBufSize;
extern CFStringRef kNStatSrcKeySndBufUsed;
extern CFStringRef kNStatSrcKeyStartAbsoluteTime;
extern CFStringRef kNStatSrcKeyTCPCCAlgorithm;
extern CFStringRef kNStatSrcKeyTCPState;
extern CFStringRef kNStatSrcKeyTCPTxCongWindow;
extern CFStringRef kNStatSrcKeyTCPTxUnacked;
extern CFStringRef kNStatSrcKeyTCPTxWindow;
extern CFStringRef kNStatSrcKeyTrafficClass;
extern CFStringRef kNStatSrcKeyTrafficMgtFlags;
extern CFStringRef kNStatSrcKeyTxBytes;
extern CFStringRef kNStatSrcKeyTxCellularBytes;
extern CFStringRef kNStatSrcKeyTxPackets;
extern CFStringRef kNStatSrcKeyTxReTx;
extern CFStringRef kNStatSrcKeyTxWiFiBytes;
extern CFStringRef kNStatSrcKeyTxWiredBytes;
extern CFStringRef kNStatSrcKeyUPID;
extern CFStringRef kNStatSrcKeyUUID;
extern CFStringRef kNStatSrcKeyVUUID;
extern CFStringRef kNStatSrcKeyVarRTT;
extern CFStringRef kNStatSrcKeyWriteProbeFailed;
extern CFStringRef kNStatSrcTCPStateCloseWait;
extern CFStringRef kNStatSrcTCPStateClosed;
extern CFStringRef kNStatSrcTCPStateClosing;
extern CFStringRef kNStatSrcTCPStateEstablished;
extern CFStringRef kNStatSrcTCPStateFinWait1;
extern CFStringRef kNStatSrcTCPStateFinWait2;
extern CFStringRef kNStatSrcTCPStateLastAck;
extern CFStringRef kNStatSrcTCPStateListen;
extern CFStringRef kNStatSrcTCPStateSynReceived;
extern CFStringRef kNStatSrcTCPStateSynSent;
extern CFStringRef kNStatSrcTCPStateTimeWait;

CFStringRef NStatSourceCopyProperty (NStatSourceRef , CFStringRef);
void NStatSourceSetDescriptionBlock (NStatSourceRef arg,  void (^)(CFDictionaryRef));
void NStatSourceSetRemovedBlock (NStatSourceRef arg,  void (^)(void));



/// End NetworkStatistics.h

// Text Color support

#define BOLD    "\033[1;1m"
#define RED     "\033[0;31m"
#define M0      "\e[0;30m"
#define CYAN    "\e[0;36m"
#define M1      "\e[0;31m"
#define GREY    "\e[0;37m"
#define M8      "\e[0;38m"
#define M9      "\e[0;39m"
#define GREEN   "\e[0;32m"
#define YELLOW  "\e[0;33m"
#define BLUE    "\e[0;34m"
#define PINK    "\e[0;35m"
#define NORMAL  "\e[0;0m"
#define DELETED  "\e[2;31m"

int g_Color = 1;

struct tcpStateColors {
	char state[16];
	char color[16];
	


} tcpStateColors[] = {

	{ "Listen",  BOLD },
	{ "Established", CYAN },
	{ "CloseWait", GREY },
	{ "Closed", DELETED},
	{ "", ""},



	};


const char *getColor(char *State) {

	// Fine to return a color ref here since we are only
	// called from a single thread
	int i = 0;

	for (i = 0 ; tcpStateColors[i].state[0]; i++) {

		if (strcmp(tcpStateColors[i].state, State) == 0) return  tcpStateColors[i].color;

	}

	return  NORMAL;
}

int g_resolveNames = 0;
//
// Utility
/// 
struct pidNetworkInfo {
	int pid;
	int  af;
	char procName[1024];
	char	provider[128];
	char	tcpState[16] ; //tcp only
	int	printed;

}  *pidNetworkInfos;
int maxProcNum = 0;

int findSlotForPid(pid_t	Pid){

	int p = 0;
	for ( p = 0; p < maxProcNum; p++) {

		if ( pidNetworkInfos[p].pid  == Pid) return p;

	}

	// If still here, assign

	pidNetworkInfos[maxProcNum].pid = Pid;
	maxProcNum++;
	return (maxProcNum - 1);
}

//
// Blocks...
//
void (^removal_callback_block) (void) = ^(void) { 

  // I'm not using this, but you can implement a callback here on source removal

};

void (^description_callback_block) (CFDictionaryRef) = ^(CFDictionaryRef Desc) { 

	// Called when another API asks for a source description, which this
	// sample does on source addition..

	CFStringRef pName = CFDictionaryGetValue(Desc, kNStatSrcKeyProcessName);
	CFNumberRef pIdentifier = CFDictionaryGetValue(Desc, kNStatSrcKeyPID);

	int pid;
	CFNumberGetValue (pIdentifier, kCFNumberSInt32Type, &pid);
	
	// Got PID, put in slot
 	int pslot = findSlotForPid(pid);

	// You can query any of the multiple kNStatSrcKeys above, I demonstrate the following

	// UDP or TCP?


	CFStringRef pProvider = CFDictionaryGetValue(Desc, kNStatSrcKeyProvider);
	CFStringGetCString(pProvider, // CFStringRef theString, 
			pidNetworkInfos[pslot].provider, // char *buffer, 
			128,	//CFIndex bufferSize, 
			kCFStringEncodingUTF8) ; // CFStringEncoding encoding)


	if (pidNetworkInfos[pslot].provider[0] == 'T') {
	
		CFStringRef tcpState = CFDictionaryGetValue(Desc, kNStatSrcKeyTCPState);
		CFStringGetCString(tcpState, // CFStringRef theString, 
			pidNetworkInfos[pslot].tcpState, // char *buffer, 
			16,	//CFIndex bufferSize, 
			kCFStringEncodingUTF8) ; // CFStringEncoding encoding)


	}

	CFStringGetCString(pName, // CFStringRef theString, 
			pidNetworkInfos[pslot].procName, // char *buffer, 
			1024,	//CFIndex bufferSize, 
			kCFStringEncodingUTF8) ; // CFStringEncoding encoding)
	
	pidNetworkInfos[pslot].printed = 0;

	//void CFDataGetBytes(CFDataRef theData, CFRange range, UInt8 *buffer);

	CFDataRef addr =  (CFDictionaryGetValue(Desc, kNStatSrcKeyRemote));
	CFIndex len = CFDataGetLength(addr);
	struct sockaddr *remoteSA = alloca (len);  // enough
	CFDataGetBytes(addr, // CFDataRef theData, 
			CFRangeMake(0,len), // CFRange range, 
			(UInt8 *)remoteSA); //UInt8 *buffer);

	pidNetworkInfos[pslot].af = remoteSA->sa_family;

	addr =  (CFDictionaryGetValue(Desc, kNStatSrcKeyLocal));
	len = CFDataGetLength(addr);
	struct sockaddr *localSA = alloca (len);  // enough
	CFDataGetBytes(addr, // CFDataRef theData, 
			CFRangeMake(0,len), // CFRange range, 
			(UInt8*)localSA); //UInt8 *buffer);

	int i = 0;
	int ll = strlen( pidNetworkInfos[pslot].provider);
	for (i = 0 ; i < ll; i ++)
	{
		 pidNetworkInfos[pslot].provider[i] = tolower( pidNetworkInfos[pslot].provider[i]);
		

	}
	pidNetworkInfos[pslot].provider[ll] = (remoteSA->sa_family == AF_INET6? '6': '4');



	char hostName[256];
	hostName[0] = '\0';
	int rc  = 0;
	int flags = 0;
	if (!g_resolveNames) { flags = NI_NUMERICHOST;}

	 rc = getnameinfo(remoteSA, // const struct sockaddr *sa, 
			      remoteSA->sa_len, // socklen_t salen, 
			      hostName, // char *host, 
			      256,	// socklen_t hostlen, 		
			     NULL, // char *serv, 
				0,  //socklen_t servlen,
				flags); // flags



	time_t ltime;

	time( &ltime );
	struct tm *ttt = localtime (&ltime);

	char timestamp[32];
	strftime(timestamp, // char *restrict s, 
		 32,	    // size_t maxsize, 
		 "%H:%M:%S",  //const char *restrict format, 
		 ttt); 	//const struct tm *restrict timeptr);


	char *l = strdup(inet_ntoa(((struct sockaddr_in *) localSA)->sin_addr));

	if (g_Color)
	{


	}

	const char *color = "";
	char *uncolor = "";
	if (g_Color )
	{

		uncolor = NORMAL;
		color = getColor(pidNetworkInfos[pslot].tcpState);
	
	
	}
	if (hostName[0]) {
		// Always true because of NI_NUMERICHOST up there

		printf ("%s%s %s\t%20s:%hu\t%30s:%-5hu  %-11s\t%d/%s%s\n",
			color,
			timestamp,
			pidNetworkInfos[pslot].provider,
			l,
			ntohs(((struct sockaddr_in *) localSA)->sin_port),
			hostName,
			ntohs(((struct sockaddr_in *) remoteSA)->sin_port),
			(pidNetworkInfos[pslot].provider[0] == 't' ? pidNetworkInfos[pslot].tcpState : ""),
			pidNetworkInfos[pslot].pid,
			pidNetworkInfos[pslot].procName,
			uncolor

			);




	}
	else  {
		// The other way:
		if (remoteSA->sa_family == AF_INET) {
		printf ("%s %s\t%20s:%hu\t%20s:%-5hu  %-11s\t%d/%s\n",
			timestamp,
			pidNetworkInfos[pslot].provider,
			l,
			ntohs(((struct sockaddr_in *) localSA)->sin_port),
			inet_ntoa (((struct sockaddr_in *) remoteSA)->sin_addr),
			ntohs(((struct sockaddr_in *) remoteSA)->sin_port),
			(pidNetworkInfos[pslot].provider[0] == 't' ? pidNetworkInfos[pslot].tcpState : ""),
			pidNetworkInfos[pslot].pid,
			pidNetworkInfos[pslot].procName );

	
		}

		if (remoteSA->sa_family == AF_INET6) {
		printf ("%s %s\t%20s:%hu\t%30s:%-5hu  %-11s\t%d/%s\n",
			timestamp,
			pidNetworkInfos[pslot].provider,
			l,
			ntohs(((struct sockaddr_in *) localSA)->sin_port),
			inet_ntoa (((struct sockaddr_in *) remoteSA)->sin_addr),
			ntohs(((struct sockaddr_in *) remoteSA)->sin_port),
			(pidNetworkInfos[pslot].provider[0] == 't' ? pidNetworkInfos[pslot].tcpState : ""),
			pidNetworkInfos[pslot].pid,
			pidNetworkInfos[pslot].procName );

	
		}

	} // Not resolve

	free (l);

	fflush(NULL);
};


void (^events_callback_block) (NStatSourceRef) = ^(NStatSourceRef Src) { 
	// Not using this either - but you can if you want. Left as an exercise
};


void (^callback_block) (void *, void *)  = ^(NStatSourceRef Src, void *arg2){

	// Arg is NWS[TCP/UDP]Source

	//NStatSourceSetRemovedBlock (Src, removal_callback_block);
	//NStatSourceSetEventsBlock (Src, events_callback_block);
	NStatSourceSetDescriptionBlock (Src, description_callback_block);
	(void) NStatSourceQueryDescription(Src);

 };


int sorted = 0;

// Not really sorting..
int 
pidComp (struct pidNetworkInfo *a, struct pidNetworkInfo *b) {

	if (!a || !b) return 0;
	if (a->pid > b->pid ) return 1;
	if (a->pid < b->pid) return -1;
	return 0;

}

#ifdef WANT_TIMER 
void 
refreshList (void *ignored) {

	
	// Display list every second. Not using this, left for the interested reader 
	// to implement

	return;
	if (!sorted) {
		qsort (pidNetworkInfos, maxProcNum, sizeof(struct pidNetworkInfo),  pidComp);

	}
	int p = 0;
	for (p = 0; p < maxProcNum; p++) {
		if (!pidNetworkInfos[p].printed) {
			if (pidNetworkInfos[p].procName[0]) {
			printf("%d:%s\n",  pidNetworkInfos[p].pid, pidNetworkInfos[p].procName);
			pidNetworkInfos[p].printed++;
				}
		}


	}

}
#endif

// Main 

int 
main (int argc, char **argv) {

   int rc; 
   char *cc = getenv("JCOLOR");
   if (cc && cc[0] == '0') g_Color = 0;

   g_resolveNames =( getenv("RESOLVE") != NULL);

   if (argc < 2) {
	fprintf(stderr,"%s [tcp/udp/all]\n", argv[0]);
	exit(1);
   }

   int wantTCP = 0;
   int wantUDP = 0;

   if (strcmp (argv[1],"tcp") == 0) {  wantTCP++;}
   if (strcmp (argv[1],"udp") == 0) {  wantUDP++;}
   if (strcmp (argv[1],"all") == 0) {  wantTCP++; wantUDP++;}
   if (!wantTCP && !wantUDP) { 
		fprintf(stderr,"I'll ignore anything you put on my argument list except 'tcp' and 'udp'\n");	exit(2);
	}


// Simple PoC, so not doing any interesting hashes or PID lookup logic
#define PID_MAX	1000000

   pidNetworkInfos = (struct pidNetworkInfo *) calloc (sizeof (struct pidNetworkInfo) , PID_MAX);

   NStatManagerRef 	nm = NStatManagerCreate (kCFAllocatorDefault,
				  &_dispatch_main_q, // DISPATCH_TARGET_QUEUE_DEFAULT,
				  callback_block);

   rc = NStatManagerSetFlags(nm, 0);
  
   // This is a really cool undocumented feature of NetworkStatistics.framework
   // Which will give you the actual control socket data output.

   int fd = open ("/tmp/nettop.trace", O_RDWR| O_CREAT | O_TRUNC);
   rc = NStatManagerSetInterfaceTraceFD(nm, fd);

   
   if (wantUDP) { rc = NStatManagerAddAllUDPWithFilter (nm, 0 , 0);}
   if (wantTCP) { rc = NStatManagerAddAllTCPWithFilter (nm, 0 , 0); }

#if WANT_TIMER
   dispatch_source_t dst = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,	// dispatch_source_type_t type, 
				0 , // uintptr_t handle, 
				0,  // unsigned long mask, 
				&_dispatch_main_q); // dispatch_queue_t queue);

   dispatch_source_set_event_handler_f (dst, refreshList);
   dispatch_source_set_timer(dst, // dispatch_source_t source, 
			     dispatch_time (DISPATCH_TIME_NOW,0), // dispatch_time_t start, 
			     1000000000, // uint64_t interval, 
			     0);//uint64_t leeway);

   dispatch_resume(dst);
#endif
   dispatch_main();

}