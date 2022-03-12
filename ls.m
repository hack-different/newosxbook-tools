#include <objc/runtime.h>
#include <libgen.h>
#include <dlfcn.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#ifdef ARM
#include <Foundation/Foundation.h> // NSObject
#endif
		CFNotificationCenterRef  CFNotificationCenterGetDistributedCenter(void);
  
//
// LSDTrip: A simple tool to demonstrate LaunchServices in OS X *and* iOS
// -------
//
// Jonathan Levin, http://NeWOSXBook.com  - @Morpheus______/@TechnoloGeeks
//
//
// License: free, but if you GitHub it or mod it, at least leave a mention and
//          don't pass it as your own code (as too many people have been doing with
//          my other demo code)
//
//    And maybe get Vol. I when it's when it's finally out.. or Tg's training
//    where this tool is used: http://Technologeeks.com/OSXRE  
//
// Compile: gcc ls.m -o lsdtrip -lobjc -framework Foundation  
//  or
// gcc-iphone -DARM ls.m -o lsdtrip -lobjc -framework Foundation -framework MobileCoreServices
//
// (that "MobileCoreServices" is required for the dlopen(3) to work)
//     
//
// To run:
// Usage: lsdtrip [apps|plugins|publicurls|privateurls] [-v]
//		app _bundle_id_ (implies -v for this app)
//
// On iOS:      privateurls|publicurls both work (via LSApplicationProxy)
//              advid  
//              launch (CFBundleIdentifier)
// On MacOS:
//              front [-v]
//              visible
//              metainfo
//		 [app|memory|notification|port|session]status
//              whohas _url_ 
//              types
//              dump <-- Try this :-)
//
// Explanation:
//  
//   Shows what are (IMHO) the most useful of the LaunchServices APIs, as exported by 
//   [Mobile]CoreServices. In iOS most of the good stuff isn't exported (t), instead wrapped
//   by LSApplicationWorkSpace, LSApplicationProxy, and friends. Even though the straight LS
//   and _LS calls are much nicer, I chose to use objective-C here in order to maintain
//   write-once-run-everywhere, and demonstrate yet again just how OS X and iOS really are
//   so similar.
//
// How this works:
//
//   Over rather nice XPC to lsd (10.11/9.0) and friends. Full explanation in MOXiI 2.
//  (was Chapter 3, "Promenade, A Tour of the OS X and iOS Frameworks", but now in Chapter 5,
//   Application Services, to be exact, where more "private" frameworks are exposed). 
// 
//  I use this tool a lot in my MOXiI trainings, and since it's one of the companion tools of 
//  MOXiI 2 Vol. 1, I thought people would appreciate a hands-on preview :-) This is just one of the many 
//  open source examples I have planned.
//
//  MOXiI 2 Volume 1 is undergoing final review (typos, typos and typos), and will be out imminently.
//
//
// Improvements:
//
//   - Can be made into pure C (i.e. not objective-C, .m), but that would make
//     the already ugly [] syntax even worse..
//
//   - There's a whole undocumented side to LaunchServices (even as an allegedly
//     "public" framework that it is) one can add here. And I left the other
//     methods out in an #if 0 block, for future use.
//
//   - The UUIDs aren't actually CFUUIDs. They're some $%#$%#$ NS...UUID, which
//     isn't compatible, so the UUIDs come with some 0s at the end. Meh.
//
//  ChangeLog:
//
//  11/04/2017 - "listen" for notifications re-enabled
//               "shmem" added
//
//  07/01/2017 - Lots more functions for MacOS (metainfo-related)
//
//  02/01/2017 - Added advertising identifier stuff
//	         Fixed dump for iOS 10 and MacOS 12 (stderr)
//
//  06/21/2016 - Added "front" (for MacOS), minor fixes

//
//  License: 
//  -------
// As will my other open sources, free - but be so kind as to at least leave the sources intact, 
// give credit, cite the MOXiI books, or something.
//
//  At the very least, don't github this and claim it as your own. People have been doing that, 
// and it's $%#$% annoying and makes me want to quit releasing these things openly.


// Ok. Let's begin:

// My own prototypes and globals:

typedef int32_t LSSessionID;

#ifndef ARM
extern CFTypeID _LSASNGetTypeID(void);
int _LSASNExtractHighAndLowParts(void *, uint32_t *H, uint32_t *L);
extern CFDictionaryRef _LSCopyApplicationInformation (signed int,
			const void *ASN,
			uint64_t Zero);
extern void * _LSCopyFrontApplication(signed int);
extern NSArray *_LSCopyApplicationArrayInFrontToBackOrder(signed int, uint64_t unknown);



#endif

CFStringRef dumpApp (NSObject *AppRef, int Verbose) ;

NSObject * workspace; // Intentionally void * so as to NOT involve @interface files
NSObject * fbSvcs;

// OS X and iOS APIs are virtually identical, but the framework name is different
#ifdef ARM
#define  CORE_SERVICE_FRAMEWORK  "/System/Library/Frameworks/MobileCoreServices.framework/MobileCoreServices"
#else
#define  CORE_SERVICE_FRAMEWORK  "/System/Library/Frameworks/CoreServices.framework/CoreServices"
#endif


void keyDumper (CFStringRef Key, void *Value, void *Nesting)
{
	// My own dumper, in simplisting format


	static char buf[1024000]; //big, but not too big - need this for MetaInfo status


	CFTypeID valueTypeID = CFGetTypeID(Value);

	if (Key ) { 
	//CFStringGetCStringPtr may fail, so opt for slow path
	CFStringGetCString(Key, // theString
			   buf, // buffer 
			   1024,// CFIndex
			   kCFStringEncodingUTF8); //CFStringEncoding

	
	}
	if (Key &&  valueTypeID != CFArrayGetTypeID()) { 
	printf("\t%s%s: ", Nesting ? "\t":"", buf);

	}

	if (valueTypeID == CFStringGetTypeID())
		{
			CFStringGetCString(Value, // theString
			   buf, // buffer 
			   1024000,// CFIndex
			   kCFStringEncodingUTF8); //CFStringEncoding
	
			printf("%s\n", buf);
			return;
		}

	
	if (valueTypeID == CFNumberGetTypeID()) 
		{
		       uint64_t valNum;
		       Boolean conv = CFNumberGetValue(Value, // CFNumberRef number,
					CFNumberGetType(Value),	 // CFNumberType theType, 
				      &valNum); // void *valuePtr);
			printf("%llu\n", valNum); // TODO - handle floats, etc

			return;
		}
	
	if (valueTypeID == CFBooleanGetTypeID())
	{
		printf("%s\n", CFBooleanGetValue(Value) ? "true" : "false");
		return;
	}

	if (valueTypeID == CFURLGetTypeID())
	{
		printf("CFURL\n");

	}

	if (valueTypeID == CFArrayGetTypeID())
	{
	
	
		int count = CFArrayGetCount(Value);
		int i = 0;
		char key[1024]; // yeah, not null term. ok. I know.
		strncpy(key,buf,1024);
		// for first element, shove a "\n"..
		for (i = 0; i < count; i++)
		{ 
			// Call ourselves recursively, but fake a key :-)
			// @TODO: proper nesting
			printf("\t%s%s[%d]: ", Nesting ? "\t\t" : "", key,i);

		
			keyDumper (NULL, // CFStringRef *Key, 
			   (void *)CFArrayGetValueAtIndex(Value, i), // void *Value, 
			   Nesting); // void *Nesting
		}
		
		
		return;
	}

	if (valueTypeID == CFDateGetTypeID())
	{
		// GOD I hate CF.
	CFStringRef enUSLocaleIdentifier = CFSTR("en_US");
CFLocaleRef enUSLocale = CFLocaleCreate(NULL, enUSLocaleIdentifier);

		CFDateFormatterRef fullFormatter = CFDateFormatterCreate
        (NULL, enUSLocale, kCFDateFormatterFullStyle, kCFDateFormatterFullStyle);

		CFStringRef dateStr = CFDateFormatterCreateStringWithDate(kCFAllocatorDefault,
			fullFormatter,
			Value);
		keyDumper(NULL,(void*)dateStr,0);
		return;


	}
	
	if (valueTypeID == CFDictionaryGetTypeID())
	{
		printf(" (dict)\n");
    		CFDictionaryApplyFunction(Value, // CFDictionaryRef theDict, 
			      keyDumper, // CFDictionaryApplierFunction CF_NOESCAPE applier,
			      (void *)1); //void *context);
		printf("\n");
		return;
		
	}


#ifndef ARM
	if (valueTypeID == _LSASNGetTypeID() )
	{
		uint32_t h, l;
		_LSASNExtractHighAndLowParts(Value, &h, &l);
		printf("0x%x-0x%x\n", h, l);
		return;

	}
		
#endif
	printf("@TODO: ");
		CFShow(Value);

}

void 
dumpDict(CFDictionaryRef dict)
{
    if(getenv("CFSHOW")) { CFShow(dict); return;}

    // @TODO: perfect SimPLIST format
    CFDictionaryApplyFunction(dict, // CFDictionaryRef theDict, 
			      keyDumper, // CFDictionaryApplierFunction CF_NOESCAPE applier,
			      NULL); //void *context);
}


CFStringRef 
serializeCFArrayToCFString (CFArrayRef Array, CFStringRef Delimiter)
{

	 if (!Array) { return (CFSTR("(null)"));}
 	 CFMutableStringRef returned = CFStringCreateMutable(kCFAllocatorDefault, // CFAllocatorRef alloc, 
                                                      4096); //CFIndex maxLength);
        

	  int len = CFArrayGetCount(Array);
	  int i = 0;
	  for (i = 0; i < len ; i++)

	    {
		CFTypeRef val = (CFTypeRef) CFArrayGetValueAtIndex(Array, i);

		if (i > 0) CFStringAppend(returned, Delimiter);

		// UGLY, I know. But PoC, people. PoC
		if (CFGetTypeID(val) == CFStringGetTypeID()) CFStringAppend(returned, val);

		else
		if (CFGetTypeID(val) == CFUUIDGetTypeID()){
			CFStringRef UUID = CFUUIDCreateString(kCFAllocatorDefault, val);
			 CFStringAppend(returned, UUID);
		
		}
		else { 
		 CFStringAppend(returned, dumpApp (val, 0));
		};
	    }


	return (returned);


} // serializeCFArrayToCFstring


CFStringRef 
dumpApp (NSObject *AppRef, int Verbose) 
{
	// App is an LSApplicationProxy object
        


	CFStringRef appID = (CFStringRef) [AppRef performSelector:@selector( applicationIdentifier)];
	CFStringRef appName =  (CFStringRef)[AppRef performSelector:@selector(localizedName)];		

	if (!appName) { appName = CFSTR("Not Set");}
	// CFBooleanRef isDeletable = (CFBooleanRef) [AppRef performSelector:@selector(isDeletable)];

 	 CFMutableStringRef out = CFStringCreateMutable(kCFAllocatorDefault, // CFAllocatorRef alloc, 
                                                      4096); //CFIndex maxLength);
 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
			CFSTR("\t%@ (%@) \n"),
		appName, appID);

	

#if 0
		// Can also use objective-C to enumerate ivars.. left as an exercise for reader :-)
		unsigned int ivarCount;
		Ivar *ivars = class_copyIvarList([AppRef class], &ivarCount);

		int i = 0;
		for (i = 0; i < ivarCount; i++)
		{
			fprintf(stderr,"\t%s: \n" , ivar_getName(ivars[i]));
			// etc.
		}
#endif
	if (Verbose)
		{
		// Dump more  - this is just a fraction of the info - 
		// jtool -v -d LSApplicationProxy /Sy*/L*/Fra*/C*Se*/Frameworks/La*S*/LaunchServices 


 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
					CFSTR("\t\tExecutable: %@\n"),
					[AppRef performSelector:@selector(bundleExecutable)]);

#ifdef ARM
		// Set on iOS. Can also use versionIdentifier here, but that requires working back from the
		// number to a version string (which LaunchServices lets you do with 
		// LSVersionNumberCopyStringRepresentation/LSVersionNumberGetXXX()
		// But why bother when you have a short version string..
 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
					CFSTR("\t\tVersion: %@\n"),
					(CFStringRef)[AppRef performSelector:@selector(shortVersionString)]);
	
#endif

		// This is apparently unset..
 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
					CFSTR("\t\tVendor Name: %@\n"),
					(CFStringRef)[AppRef performSelector:@selector(vendorName)]);



/*
 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
					CFSTR("\t\tMach-O UUIDs: %@\n"),
					serializeCFArrayToCFString((CFArrayRef)[AppRef performSelector:@selector(machOUUIDs)], CFSTR(",")));
	
*/

 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
					CFSTR("\t\tDisk Usage (Static): %@\n"),
					(CFStringRef)[AppRef performSelector:@selector(staticDiskUsage)]);
		

#if 0
		// This apparently doesn't work in 9.2 anymore. Not sure about this..
 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
					CFSTR("\t\tDisk Usage (Dynamic): %@\n"),
					(CFStringRef)[AppRef performSelector:@selector(dynamicDiskUsage)]);
		
#endif

		CFArrayRef UIBackgroundModes = (CFArrayRef) [AppRef performSelector: @selector(UIBackgroundModes)];
		

		// This is a CFArray
		if (!CFArrayGetCount(UIBackgroundModes)) { 
			CFStringAppend (out,CFSTR("\t\tno BackgroundModes"));
			}

		else
 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
						CFSTR("\t\t   BackgroundModes: %@"),
						serializeCFArrayToCFString((CFArrayRef)UIBackgroundModes, CFSTR(",")));

		CFStringAppend(out, CFSTR("\n"));

#ifdef ARM
		// Only on iOS
 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
					CFSTR("\t\tApplication DSID: %@\n"),
					(CFStringRef)[AppRef performSelector:@selector(applicationDSID)]);
		
		

 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
					CFSTR("\t\tPurchaser DSID: %@\n"),
					(CFStringRef)[AppRef performSelector:@selector(purchaserDSID)]);



 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
					CFSTR("\t\tDownloader DSID: %@\n"),
					(CFStringRef)[AppRef performSelector:@selector(downloaderDSID)]);
#endif


#if 0
		uint64_t  modTime = (uint64_t)([AppRef performSelector:@selector( bundleModTime)]); 
		fprintf(stderr, "\t\tBundle Mod Time: %llu\n", modTime);
#endif
		

		int  cont = (int)([AppRef performSelector:@selector( isContainerized)]); 
		int  restricted  = (int)([AppRef performSelector:@selector( isRestricted)]); 

		CFStringAppendFormat(out,
				     NULL,
				     CFSTR("\t\tContainerized: %@\n\t\tRestricted: %@\n"),
				(cont ? CFSTR("YES (q.v. App-Store Receipt URL for container)") : CFSTR("NO")),
				(restricted ? CFSTR("YES") : CFSTR("NO")));



 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
					CFSTR("\t\tApp Store Receipt URL: %@\n"),
					(CFStringRef)[AppRef performSelector:@selector( appStoreReceiptURL)]);


		// These are from LSBundleProxy, which is the parent of LSApplicationProxy
 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
					CFSTR("\t\tContainer URL: %@\n"),
					[AppRef performSelector:@selector(containerURL)]);

		CFDictionaryRef entitlements = (CFDictionaryRef) [AppRef performSelector:@selector(entitlements)];
		
		if (entitlements &&  CFDictionaryGetCount(entitlements) ) 
		{

		CFDataRef xml = CFPropertyListCreateXMLData(kCFAllocatorDefault,
                                                (CFPropertyListRef)entitlements);
	
		CFStringRef xmlAsString = CFStringCreateFromExternalRepresentation(NULL, xml, kCFStringEncodingUTF8);

   		 if (xmlAsString) {
 		CFStringAppendFormat(out, // CFMutableStringRef theString, 
                             NULL,   // CFDictionaryRef formatOptions, 
					CFSTR("\t\tEntitlements:\n-----\n %@\n-----\n"),
					xmlAsString);
		
			  }
		else { CFStringAppend (out, CFSTR("\t\tEntitlements: Internal error\n"));}


		} // entitlements
		else 
			CFStringAppend(out,CFSTR("\t\tEntitlements: None\n")); 

	} // Verbose
	
	return (out);
}


/*
 *
 * Soooo much more, courtesy of JTool. Feel free to extend these on your own time:
 *

-[LSApplicationProxy applicationIdentifier]:
-[LSApplicationProxy roleIdentifier]:
-[LSApplicationProxy bundleModTime]:
-[LSApplicationProxy registeredDate]:
-[LSApplicationProxy deviceFamily]:
-[LSApplicationProxy minimumSystemVersion]:
-[LSApplicationProxy sdkVersion]:
-[LSApplicationProxy applicationType]:
-[LSApplicationProxy itemName]:
-[LSApplicationProxy sourceAppIdentifier]:
-[LSApplicationProxy companionApplicationIdentifier]:
-[LSApplicationProxy applicationVariant]:
-[LSApplicationProxy storeCohortMetadata]:
-[LSApplicationProxy shortVersionString]:
-[LSApplicationProxy preferredArchitecture]:
-[LSApplicationProxy familyID]:
-[LSApplicationProxy groupContainers]:
-[LSApplicationProxy directionsModes]:
-[LSApplicationProxy UIBackgroundModes]:
-[LSApplicationProxy audioComponents]:
-[LSApplicationProxy externalAccessoryProtocols]:
-[LSApplicationProxy VPNPlugins]:
-[LSApplicationProxy plugInKitPlugins]:
-[LSApplicationProxy appTags]:
-[LSApplicationProxy requiredDeviceCapabilities]:
-[LSApplicationProxy deviceIdentifierForVendor]:
-[LSApplicationProxy ODRDiskUsage]:
-[LSApplicationProxy storeFront]:
-[LSApplicationProxy externalVersionIdentifier]:
-[LSApplicationProxy betaExternalVersionIdentifier]:
-[LSApplicationProxy appStoreReceiptURL]:
-[LSApplicationProxy installProgress]:
-[LSApplicationProxy installProgressSync]:
-[LSApplicationProxy resourcesDirectoryURL]:
-[LSApplicationProxy privateDocumentIconNames]:
-[LSApplicationProxy setPrivateDocumentIconNames:]:
-[LSApplicationProxy privateIconsDictionary]:
-[LSApplicationProxy privateDocumentIconAllowOverride]:
-[LSApplicationProxy setPrivateDocumentIconAllowOverride:]:
-[LSApplicationProxy iconDataForVariant:]:
-[LSApplicationProxy privateDocumentTypeOwner]:
-[LSApplicationProxy setPrivateDocumentTypeOwner:]:
-[LSApplicationProxy localizedName]:
-[LSApplicationProxy localizedShortName]:
-[LSApplicationProxy localizedNameForContext:]:
-[LSApplicationProxy iconIsPrerendered]:
-[LSApplicationProxy fileSharingEnabled]:
-[LSApplicationProxy profileValidated]:
-[LSApplicationProxy isAdHocCodeSigned]:
-[LSApplicationProxy isPlaceholder]:
-[LSApplicationProxy isAppUpdate]:
-[LSApplicationProxy isNewsstandApp]:
-[LSApplicationProxy isRestricted]:
-[LSApplicationProxy supportsAudiobooks]:
-[LSApplicationProxy supportsExternallyPlayableContent]:
-[LSApplicationProxy supportsOpenInPlace]:
-[LSApplicationProxy hasSettingsBundle]:
-[LSApplicationProxy isInstalled]:
-[LSApplicationProxy isWhitelisted]:
-[LSApplicationProxy isBetaApp]:
-[LSApplicationProxy isPurchasedReDownload]:
-[LSApplicationProxy isWatchKitApp]:
-[LSApplicationProxy hasMIDBasedSINF]:
-[LSApplicationProxy missingRequiredSINF]:
-[LSApplicationProxy isEqual:]:
-[LSApplicationProxy hash]:
-[LSApplicationProxy description]:
-[LSApplicationProxy iconStyleDomain]:
-[LSApplicationProxy userActivityStringForAdvertisementData:]:
-[LSApplicationProxy populateNotificationData]:
-[LSApplicationProxy itemID]:
-[LSApplicationProxy installType]:
-[LSApplicationProxy originalInstallType]:
-[LSApplicationProxy groupIdentifiers]:
-[LSApplicationProxy teamID]:
-[LSApplicationProxy isContainerized]:
*/


struct appInfoInShmem {
	uint32_t ASN;
	uint32_t seed;
	uint32_t flags;
	uint32_t visibleIndex;
	uint32_t pid;
};

struct LSShmem {
	uint32_t	version;
	uint32_t	sizeInPages;
	uint32_t	session;
	uint32_t	zero;
	uint32_t	debugLogLevel;
	uint32_t	zero1;
	uint64_t	alsoZero;
	uint32_t	frontASNSeed;
	uint32_t	menuBarOwnerASNSeed;
	uint32_t	applicationListSeed;
	uint32_t	pendingApplicationListSeed;
	uint32_t	visibleApplicationListSeed;
	uint32_t	applicationInformationSeed;
	uint64_t	unknown[3];
	uint32_t	frontASN;
	uint32_t	menuBarOwnerASN;
	uint32_t	systemProcessASN;
	uint32_t	systemLauncherASN;
	uint64_t	nevermind[5];
	uint32_t	numPids;
	uint32_t	whatever;
	struct appInfoInShmem	appList[0];

/*
 menuBarOwnerASN="Terminal" ASN:0x0-0xa00a: 
 systemProcessASN="loginwindow" ASN:0x0-0x1001: 
 systemLauncherASN=ASN:0x0-0x0: 
 nextAppToBringForwardASNLowHalf=ASN:0x0-0x0: 
 expectedFrontASNLowerHalf=ASN:0x0-0x0: 
 systemUIPresentationMode=0 "Normal" systemUIPresentationOptions=0
 launchProgress=0 
 launchProgessUserActivityCount=0
 processes = #37
*/


};

#ifndef ARM
void dumpShmem (struct LSShmem *Shmem)
{
	printf("Shmem at address %p (%d pages = 0x%x):\n", Shmem,
				Shmem->sizeInPages, Shmem->sizeInPages * 0x1000);
	printf("Session: %d\n", Shmem->session);
 	// lockCount=0
	printf("DebugLogLevel: %d\n", Shmem->debugLogLevel);
	printf("frontASNSeed: %d\n", Shmem->frontASNSeed);
	printf("menuBarOwnerASNSeed: %d\n", Shmem->menuBarOwnerASNSeed);
	printf("applicationListSeed: %d\n", Shmem->applicationListSeed);
	printf("pendingApplicationListSeed: %d\n", Shmem->pendingApplicationListSeed);
	printf("visibleApplicationListSeed: %d\n", Shmem->visibleApplicationListSeed);
	printf("applicationInformationSeed: %d\n", Shmem->applicationInformationSeed);
	printf("frontASN: 0x%x\n", Shmem->frontASN);
	printf("systemProcessASN: 0x%x\n", Shmem->systemProcessASN);
	printf("systemLauncherASN: 0x%x\n", Shmem->systemLauncherASN);
	
	printf("Processes: %d\n", Shmem->numPids);
	int p = 0;


	for (p = 0; p < Shmem->numPids; p++)
	{
		printf("\t App: ASN 0x%x PID: %d Visible Index: %d Seed: %d, Flags: 0x%x\n",
		Shmem->appList[p].ASN,
		Shmem->appList[p].pid,
		Shmem->appList[p].visibleIndex,
		Shmem->appList[p].seed,
		Shmem->appList[p].flags);

	}
	




} // dump Shmem
#endif


void 
dumpURL (CFStringRef URL, int Verbose) 
{
	
 	CFMutableStringRef out = CFStringCreateMutable(kCFAllocatorDefault, // CFAllocatorRef alloc, 
                                                      4096); //CFIndex maxLength);
	
	CFStringAppend(out, URL);
	if (Verbose) {

        CFArrayRef appsForURL = (CFArrayRef) [workspace performSelector:@selector(applicationsAvailableForHandlingURLScheme:) withObject:(id)URL];

	if (!appsForURL) { 
		// Maybe it's a document type..
	// appsForURL = (CFArrayRef) [workspace performSelector:@selector(applicationsAvailableForOpeningDocument:) withObject:(id)URL];

		}
	// if still no apps, then no claim.
	if (!appsForURL)
		{
		fprintf(stderr,"No app has claimed this URL\n"); exit(1);
		}

	
	// This is a CFArray
	if (CFGetTypeID(appsForURL) != CFArrayGetTypeID())
	{
		fprintf(stderr, "Was expecting a CFArray of Apps!\n");
		exit(2);
	}

	if (!CFArrayGetCount (appsForURL)) CFStringAppend(out,CFSTR(" - Not claimed by anyone"));
	else {

	 CFStringRef apps = serializeCFArrayToCFString (appsForURL, CFSTR("\n\t\t\t"));

	 CFStringAppend (out, apps);
	 CFRelease (apps);
	}
	}

	CFShow(out);
	CFRelease(out);
}	
void 
dumpPlugin (NSObject *PluginRef, int Verbose) 
{

	CFStringRef pluginName = (CFStringRef) [PluginRef performSelector:@selector( localizedName)];
	CFStringRef pluginID = (CFStringRef) [PluginRef performSelector:@selector( pluginIdentifier)];
	CFUUIDRef pluginUUID =  (CFUUIDRef)[PluginRef performSelector:@selector(pluginUUID)];		

	CFStringRef out = CFStringCreateWithFormat(kCFAllocatorDefault,
			NULL, CFSTR("\t%@ (%@) - %@"),
			pluginName,pluginID, CFUUIDCreateString(kCFAllocatorDefault, pluginUUID));

	CFShow(out);


} // end Dump

void monitorCallback()
{
	printf("In callback\n");
}



int notificationCallbackFunc(int a, CFDictionaryRef *Notif, void *todo1, void *x, uint64_t xx, void *y) {

	
	printf("Notification Received:\n");
	dumpDict(Notif);

	return 0;
}

void 
usage (char *ProgName)
{

		#ifdef ARM
		fprintf(stderr, "Usage: %s [apps|plugins|publicurls|privateurls] [-v]\n", basename(ProgName));
		fprintf(stderr, "                app _bundle_id_ (implies -v for this app)\n");
		fprintf(stderr, "                launch _bundle_id_\n");
		fprintf(stderr, "                advid [clear]\n");
		#else
		fprintf(stderr, "Usage: %s [apps|plugins] [-v]\n", basename(ProgName));
		fprintf(stderr, "                app _bundle_id_ (implies -v for this app)\n");
		fprintf(stderr, "                front [-v]\n");
		fprintf(stderr, "                visible\n");
		fprintf(stderr, "                listen          (Get app launch/foreground notifications - cool)\n");
		fprintf(stderr, "                shmem\n");
		fprintf(stderr, "                metainfo\n");
		fprintf(stderr, "                [app|memory|notification|port|session]status\n");
		#endif
		fprintf(stderr, "                whohas _url_ \n");
		fprintf(stderr, "                types\n");
		fprintf(stderr, "                dump\n");
		fprintf(stderr, "\nSet CFSHOW=1 to use CFshow in place of J's dumping function (which is still a work in progress)\n");
		exit(0);
} // usage

int 
main (int argc, char **argv)
{

  int verbose = 0;

  // So - why dup? because CFShow(), which I use extensively, write to stderr.
  // And I really CANNOT stand the whole CFStringToCstr $%#$%#$ just to get a stupid
  // printout! So instead, I save stderr, and then reopen stderr to stdout. This
  // way, the output can be grep(1)-ed easily.

  dup2(2,3);
  dup2(1,2);

  if (argc < 2)
	{
		usage(argv[0]);

		}

   if (argv[2] && strcmp(argv[2], "-v")==0) verbose++;

   // Getting the LS* classes and functions we need here:
   // ---------------------------------------------------
   Class LSApplicationWorkspace_class = objc_getClass("LSApplicationWorkspace");
   if (!LSApplicationWorkspace_class) {fprintf(stderr,"Unable to get Workspace class\n"); exit(1);}
   workspace = [LSApplicationWorkspace_class performSelector:@selector (defaultWorkspace)];
   if (!workspace) {fprintf(stderr,"Unable to get Workspace\n"); exit(2);}

   void *CS_handle = dlopen (CORE_SERVICE_FRAMEWORK, RTLD_NOW);
   if (!CS_handle) { fprintf(stderr,"Can't find %s!\n", CORE_SERVICE_FRAMEWORK); exit(2); };


#ifdef ARM


   if (strcmp(argv[1],"running") == 0)
	{
#if 0
   		Class fbSvcs_class = objc_getClass("FBSSystemService");

	   	if (!fbSvcs_class) { fprintf(stderr,"Unable to get Workspace\n"); exit(2);}
   		fbSvcs = [fbSvcs_class performSelector:@selector (sharedService)];
		printf("GOT SVCS %p\n", fbSvcs);

#endif
		printf("Not yet\n");
		exit(0);
	} 
#endif
   if ( (strcmp(argv[1],"apps") == 0) || (strcmp(argv[1], "app") == 0))

	{

		CFStringRef wantedBundleID = NULL;

		
		if (strcmp(argv[1], "apps")) // that is, not all apps
		{
			// expecting a bundle identifier here
		
			if (!argv[2]) { fprintf(stderr,"app option requires a specific bundle id. Try 'apps' to get a list\n");
					exit(5); }
		
			wantedBundleID = CFStringCreateWithCString(kCFAllocatorDefault, // CFAllocatorRef alloc,
					 	    argv[2],  // const char *cStr, 
						    kCFStringEncodingUTF8); //CFStringEncoding encoding);
		}

		CFArrayRef apps =  (CFArrayRef) [workspace performSelector:@selector(allApplications)];
		// This is a CFArray
		if (CFGetTypeID(apps) != CFArrayGetTypeID())
		{
			fprintf(stderr, "Was expecting a CFArray of Apps!\n");
			exit(2);
		}
	
		int len = CFArrayGetCount(apps);
		int i = 0;
		for (i = 0; i < len ; i++)
		{
			CFTypeRef app = CFArrayGetValueAtIndex(apps,i);

			// Got app: Dump if want all, or if matches id.
			// I'm sure there's some Workspace method I missed to get an app by bundle ID, instead
			// of iterating over all of them..

			CFStringRef appID = (CFStringRef) [(id)app performSelector:@selector(applicationIdentifier)];
			
			if (appID && wantedBundleID)
			{
				if (CFEqual(appID, wantedBundleID))
				{
				CFStringRef dump = dumpApp(app, 1);
				CFShow(dump);
				CFRelease(dump);
				exit(0); // only one match here.
				}
			}
			else
			{
			CFStringRef dump = dumpApp(app, verbose);
			CFShow(dump);
			CFRelease(dump);


			}
		}

		if ( wantedBundleID) { 
			fprintf(stderr,"Application with Bundle ID %s not found. Try '%s apps' first, and remember case-sensitivity\n",
				argv[2], argv[0]);
			exit(2);
			}

		exit(0);
	}

   if (strcmp(argv[1], "publicurls") == 0)
	{
#ifdef ARM
		CFArrayRef puburls =  (CFArrayRef) [workspace performSelector:@selector(publicURLSchemes)];
	
	
		// This is a CFArray
		if (CFGetTypeID(puburls) != CFArrayGetTypeID())
		{
			fprintf(stderr, "Was expecting a CFArray of publicURLSchemes!\n");
			exit(2);
		}
	
		int len = CFArrayGetCount(puburls);
		int i = 0;
		for (i = 0; i < len ; i++)
		{
			CFStringRef url = CFArrayGetValueAtIndex(puburls,i);
			dumpURL(url, verbose);



		}
#else
		fprintf(stderr,"Not implemented on MacOS.. yet\n");
#endif
		exit(0);

	}

   if (strcmp(argv[1], "privateurls") == 0)
	{
#ifndef ARM

		// MacOS's LSApplicationWorkSpace does not respond to private URLs..

		//	 @TODO:__LSCopyAllApplicationURLs
		printf("Not implemented on MacOS.. yet\n");
	

#else
		CFArrayRef (privurls) = (CFArrayRef) [workspace performSelector:@selector(privateURLSchemes)];
		// This is a CFArray
		if (CFGetTypeID(privurls) != CFArrayGetTypeID())
		{
			fprintf(stderr, "Was expecting a CFArray of privateURLSchemes!\n");
			exit(2);
		}
	
		int len = CFArrayGetCount(privurls);
		int i = 0;
		for (i = 0; i < len ; i++)
		{
			CFStringRef url = CFArrayGetValueAtIndex(privurls,i);
			dumpURL(url, verbose);



		}
#endif
		exit(0);

	}


   if (strcmp(argv[1], "plugins") == 0)
	{
		CFArrayRef plugins = (CFArrayRef) [workspace performSelector:@selector(installedPlugins)];
		// This, too, is a CFArray
		if (CFGetTypeID(plugins) != CFArrayGetTypeID())
		{
			fprintf(stderr, "Was expecting a CFArray of plugins!\n");
			exit(2);
		}
	
		int len = CFArrayGetCount(plugins);
		int i = 0;
		for (i = 0; i < len ; i++)
		{
			CFTypeRef plugin = CFArrayGetValueAtIndex(plugins,i);
			dumpPlugin(plugin, verbose);



		}
		exit(0);
	}

   if (strcmp(argv[1], "types") == 0)
	{
		// I resort to dlopen/dlsym here to make linking simpler. You don't have to.

		typedef CFArrayRef (*UTCopyDeclaredTypeIdentifiersFunc)(void);

		UTCopyDeclaredTypeIdentifiersFunc UTCopyDeclaredTypeIdentifiers 
				  = dlsym (CS_handle, "_UTCopyDeclaredTypeIdentifiers");

        	CFArrayRef utis =  UTCopyDeclaredTypeIdentifiers();
	
		// As usual, this is an array..
		int len = CFArrayGetCount(utis);
		int i = 0;
		for (i = 0; i < len ; i++)
		{
			// Seriously, AAPL - couldn't you find a better acronym?
			CFTypeRef uti = CFArrayGetValueAtIndex(utis,i);
			CFShow(uti);

		}
		exit(0);
		


	}

   if (strcmp(argv[1], "whohas") == 0)
	{

	if (!argv[2]) {
		fprintf(stderr,"whohas: Option requires an URL Scheme or UTI as an argument!\n");
		exit(3);
		}

		

	CFStringRef url = CFStringCreateWithCString(kCFAllocatorDefault, // CFAllocatorRef alloc,
					 	    argv[2],  // const char *cStr, 
						    kCFStringEncodingUTF8); //CFStringEncoding encoding);

	dumpURL(url,1);


		exit(0);
	}



#if 0
	// ignore this part for now
	if (strcmp(argv[1],"claims") == 0)
	{

		typedef CFStringRef (*LSCopyClaimedActivityIdentifiersAndDomainsFunc)(CFStringRef, CFStringRef);

		LSCopyClaimedActivityIdentifiersAndDomainsFunc 
		 LSCopyClaimedActivityIdentifiersAndDomains = dlsym (CS_handle, "_LSCopyClaimedActivityIdentifiersAndDomains");

		if (!LSCopyClaimedActivityIdentifiersAndDomains )
		{
			fprintf(stderr,"Unable to find LSCopyClaimedActivityIdentifiersAndDomains ");
			exit(3);
		}
		
		CFStringRef result = LSCopyClaimedActivityIdentifiersAndDomains (NULL, NULL);
		CFShow(result);
		exit(0);


	}
#endif



	if (strcmp(argv[1],"advid") == 0)
	{

	   if ((argc ==3) && strcmp(argv[2], "clear") == 0) {
		   (void) [workspace performSelector:@selector(clearAdvertisingIdentifier) ];
			printf("advertising identifier should be clear\n");
		}
	    else {
		// just show
		CFStringRef advid = [workspace performSelector:@selector(deviceIdentifierForAdvertising)];

		CFShow(advid);

		}

	exit(0);

	}

#ifdef ARM
	if (strcmp(argv[1],"open") == 0)
	{

		if (!argv[2]) {
		fprintf(stderr,"open: Option requires an argument!\n"); exit(3);
		}
	    CFStringRef theURLs = CFStringCreateWithCString(kCFAllocatorDefault, // CFAllocatorRef alloc,
					 	    argv[2],  // const char *cStr, 
						    kCFStringEncodingUTF8); //CFStringEncoding encoding);
		
		CFURLRef theURL;
	theURL = CFURLCreateWithString(kCFAllocatorDefault, // CFAllocatorRef allocator
				theURLs, //CFStringRef URLString, 
				NULL);   // CFURLRef baseURL);

		printf("CREATED URL %p\n", theURL);
		
	   int t =  (int) [workspace performSelector:@selector(openURL:) withObject:(id) theURL ];
	   fprintf(stderr, "%s %s (RC: %d)\n",
			t ? "opened" : "Unable to open",
		 	argv[2], t);

#if 0

	extern int
	LSOpenCFURLRef(
  CFURLRef                         inURL,
  __nullable CFURLRef *__nullable  outLaunchedURL)              ;

	LSOpenCFURLRef(theURL, NULL);



#endif
	exit(0);


	}
#endif
	if (strcmp(argv[1],"launch") == 0)
	{


		if (!argv[2]) {
		fprintf(stderr,"launch: Option requires an argument!\n"); exit(3);
		}
	    CFStringRef bundleID = CFStringCreateWithCString(kCFAllocatorDefault, // CFAllocatorRef alloc,
					 	    argv[2],  // const char *cStr, 
						    kCFStringEncodingUTF8); //CFStringEncoding encoding);
	   int t =  (int) [workspace performSelector:@selector(openApplicationWithBundleID:) withObject:(id) bundleID ];



	   fprintf(stderr, "%s %s\n",
			t ? "Launched" : "Unable to launch",
		 	argv[2]);

		 exit(0);

	}



#ifndef ARM

// This only works on MacOS
// because A) API doesn't exist in *OS
//         B) there's no "visible application ordering" in *OS, to begin with.

	if (strcmp(argv[1], "visible") == 0)
	{
		
		NSArray *apps = _LSCopyApplicationArrayInFrontToBackOrder(-2, 0);
		// Apps is an array of LSASN types
		int a = 0;
		for (a = 0; a < [apps count]; a++)
			{
			   void *asn = [apps  objectAtIndex:a];
			 
			   CFDictionaryRef dict = _LSCopyApplicationInformation (-2, asn,0);
			   CFStringRef bn = CFDictionaryGetValue(dict, CFSTR("CFBundleName"));
			   CFStringRef  bp = CFDictionaryGetValue(dict, CFSTR("CFBundleExecutablePath"));
			   // ShowASN (ignoring return value here)
			   uint32_t high, low;

			   //int _LSASNExtractHighAndLowParts(void *, uint32_t *H, uint32_t *L);
			   (void) _LSASNExtractHighAndLowParts (asn, &high, &low);

			   fprintf(stderr,"0x%x-0x%-6x ", high, low);
			
			  // CFShow(bn);
			   CFShow(bp);
			
			}
		exit(0);

	}
	if (strcmp(argv[1], "front") == 0)
	{
		// If you wanted the front most application you could also
		// print the first entry in the array from "Visible" (that is,
		// _LSCopyApplicationArrayInFrontToBackOrder)
		CFStringRef front = _LSCopyFrontApplication(-2);
		if (!front) {
			fprintf(stderr,"Can't get front application?\n"); exit(2);
		}
		if (_LSASNGetTypeID() == CFGetTypeID(front))
		{
			// Now get info
			CFDictionaryRef dict = _LSCopyApplicationInformation (-2, front,0);
			if (verbose)
				{
				dumpDict(dict);
				}
			else
			{
			   CFStringRef dn = CFDictionaryGetValue(dict, CFSTR("CFBundleName"));
			   CFShow(dn);
	
			}

			exit(0);
		}
		else
		{
			fprintf(stderr,"Got front application but not an LSASN type.. Has AAPL changed the API or something?\n"); exit(3);
		

		}



		exit(0);
	}


	extern CFDictionaryRef _LSCopyMetaApplicationInformation(LSSessionID id, CFArrayRef *What);


	
	if (strcmp(argv[1], "metainfo") == 0)
	{


		// A null pointer for What gets you a raw dict.
		CFDictionaryRef meta = _LSCopyMetaApplicationInformation(-2,0);
		if (!meta) { printf("NO META\n"); exit(0);}
	
		dumpDict(meta);
		exit(0);

	}

	if (strstr(argv[1], "status")) {

		CFStringRef vals[4];
		vals[0] = CFSTR("Status");
		vals[1] = NULL;

		// A non null pointer for What (NS/CFArray)  gets you a specific dict
		// which is already human-readable
		//
		if (strcmp(argv[1], "appstatus") == 0) vals[1] = CFSTR("ApplicationStatus");
		if (strcmp(argv[1], "notificationstatus") == 0) vals[1] = CFSTR("NotificationStatus");
		if (strcmp(argv[1], "portstatus") == 0) vals[1] = CFSTR("PortStatus");
		if (strcmp(argv[1], "sessionstatus") == 0) vals[1] = CFSTR("SessionStatus");
		// Only supported in DEBUG builds :-(
		if (strcmp(argv[1], "memorystatus") == 0) vals[1] = CFSTR("MemoryStatus");

		if (!vals[1]) { fprintf(stderr,"What kind of status is %s?\n", argv[1]); exit(5);}

		CFArrayRef what =  CFArrayCreate(kCFAllocatorDefault, // CFAllocatorRef allocator,
			(const void **)		vals, // const void **values, 
					2,    // CFIndex numValues, 
					&kCFTypeArrayCallBacks); //const CFArrayCallBacks *callBacks);

		CFDictionaryRef meta = _LSCopyMetaApplicationInformation(-2,what);

		dumpDict(meta);
		exit(0);

	}
		typedef void *LSNotificationReceiverRef;

	if (strcmp(argv[1],"shmem") == 0)
	{
		extern void * _LSDebugGetSharedMemoryPageAddress(LSSessionID);
		
		void *shmem = _LSDebugGetSharedMemoryPageAddress(-2);
		if (shmem) {
			dumpShmem(shmem);
		}



		exit(0);
	}
	extern LSNotificationReceiverRef _LSScheduleNotificationFunction(LSSessionID,
			void *callback, // actually a func pointer
			uint64_t  zero,
			NSMutableArray *context, // RCX
			CFRunLoopRef CFRunLoop,
			const CFRunLoopMode CFRunLoopMode);
		
	if (strcmp(argv[1],"listen") == 0)
	{

		
		CFRunLoopRef rl = CFRunLoopGetCurrent();

		// If you don't give an array, even empty, you get RC 775
		NSMutableArray *notifs = [[NSMutableArray alloc ] init];


		LSNotificationReceiverRef ref = 
		_LSScheduleNotificationFunction(-2,
			notificationCallbackFunc,
			0,
			notifs, // 0, //notifs,
			rl,
			NULL); //kCFRunLoopDefaultMode); //CFRunLoopCopyCurrentMode(rl));

		

		// 11/1/2017 This was missing from earlier version
		
		extern _LSModifyNotification(LSNotificationReceiverRef ref,
					    int numKeys,
					    uint32_t	*mask,
					    uint32_t   unknown,
					    uint32_t   unknown1,
					    uint32_t   unknown2);


		int mask=-1; // get all
		_LSModifyNotification(ref, // 
				      1,  // uint32
				     &mask,
				      0,
				      0, 0);
	
					



		// If you want to actually see the XPC notification, put a breakpoint on:
//LaunchServices`LSNotificationReceiver::receiveNotificationFromServer(_xpc_connection_s*, void*) 


		// Need a runloop for this, so run.

		CFRunLoopRun();
		/* --- */
		printf("Not reached ....\n");
		exit(0);

	}
#endif

	// NOT YET
	if (strcmp(argv[1],"monitor-notyet") == 0)
	{

		// You can do this with an LSApplicationWorkspaceObserver, but that requires
		// objective C, in order to implement the callback messages. Much easier
		// in a pure C solution to do this by directly accessing the Notification center

	//	extern void *CFNotificationCenterGetDistributedCenter(void);
		CFNotificationCenterRef distCent = CFNotificationCenterGetDistributedCenter();


		//CFStringRef name = CFSTR("com.lsinstallprogress.appcontrols.pause"); // com.apple.LaunchServices.applicationStateChanged");
		CFStringRef name = CFSTR("com.apple.LaunchServices.applicationStateChanged");
		extern void CFNotificationCenterAddObserver(CFNotificationCenterRef center, const void *observer, CFNotificationCallback callBack, CFStringRef name, const void *object, CFNotificationSuspensionBehavior suspensionBehavior);

		void *observer;
		void *monitorCallback;
		CFNotificationCenterAddObserver(distCent,
			NULL, //observer,
			monitorCallback,
			name,
			NULL,
			CFNotificationSuspensionBehaviorDeliverImmediately);
			
	printf("Runloop\n");
		CFRunLoopRun();

#if 0
0000000000166c5        callq   0x130cc4 ## symbol stub for: _CFNotificationCenterGetDistributedCenter
00000000000166ca        leaq    _LSApplicationWorkspaceObserverCallback(%rip), %rdx
00000000000166d1        leaq    0x16e308(%rip), %rcx ## Objc cfstring ref: @"com.lsinstallprogress.appcontrols.cancel"
00000000000166d8        xorl    %r8d, %r8d
00000000000166db        movl    $0x4, %r9d
00000000000166e1        movq    %rax, %rdi
00000000000166e4        movq    %rbx, %rsi
00000000000166e7        callq   0x130cb8 ## symbol stub for: _CFNotificationCenterAddObserver
#endif


		
		sleep(100);
		exit(0);
	

	}
	// And this is the real fun part:
	//
	// The little known "lsregister" utility in OS X is closed source, and un-man(1)ned,
	// But one of its coolest features is "dump" - to dump the LaunchServices Database.
	// Turns out this is a *single* line of code. And guess what -- it works on iOS too :-)
	//
	if (strcmp(argv[1], "dump") == 0)
	{

	typedef int (*LSDisplayDataFunc)(FILE *, void*);
        LSDisplayDataFunc       LSDisplayData;

        // If you want to get the raw format of the file, you can dump with this, instead:
        //extern _LSDisplayRawStoreData(char *, int);
        //_LSDisplayRawStoreData("/var/mobile/Library/Caches/com.apple.LaunchServices-134.csstore", (void *) 0xff);


        LSDisplayData = dlsym (CS_handle, "_LSDisplayData");

	if (!LSDisplayData) { fprintf(stderr, "Can't find LSDisplayData! Has Apple removed it by now? :-P"); exit(1);}

        // The argument expected here is likely the name of the CoreServices (LaunchServices)
        // DataStore - in iOS "/var/mobile/Library/Caches/com.apple.LaunchServices-###.csstore"
        // but turns out NULL <del>works</del> on both OS X and iOS!

	// Until iOS 10, that is - wherein you need FILE * - so I use stderr

  	int rc =  LSDisplayData(stderr,NULL);
	if (rc != 0) { fprintf(stderr,"LSDisplayData returned %d\n",rc);}
	exit(rc);

	}

	// Still here? Usage
	fprintf(stderr,"I don't understand '%s'\n", argv[1]);
	usage(argv[0]);

#if 0
0000000183063cb8 t +[LSApplicationWorkspaceObserver supportsSecureCoding]
0000000183063cc0 t -[LSApplicationWorkspaceObserver encodeWithCoder:]
0000000183063d00 t -[LSApplicationWorkspaceObserver init]
0000000183063d74 t -[LSApplicationWorkspaceObserver initWithCoder:]
0000000183063e08 t -[LSApplicationWorkspaceObserver dealloc]
0000000183063e64 t -[LSApplicationWorkspaceObserver applicationInstallsDidStart:]
0000000183063fa0 t -[LSApplicationWorkspaceObserver applicationInstallsDidChange:]
00000001830640dc t -[LSApplicationWorkspaceObserver applicationInstallsDidUpdateIcon:]
0000000183064218 t -[LSApplicationWorkspaceObserver applicationsDidInstall:]
0000000183064354 t -[LSApplicationWorkspaceObserver applicationsWillInstall:]
0000000183064490 t -[LSApplicationWorkspaceObserver applicationsWillUninstall:]
00000001830645cc t -[LSApplicationWorkspaceObserver applicationsDidFailToInstall:]
0000000183064708 t -[LSApplicationWorkspaceObserver applicationsDidFailToUninstall:]
0000000183064844 t -[LSApplicationWorkspaceObserver applicationsDidUninstall:]
0000000183064980 t -[LSApplicationWorkspaceObserver applicationInstallsArePrioritized:arePaused:]
0000000183064be8 t -[LSApplicationWorkspaceObserver applicationInstallsDidPause:]
0000000183064d24 t -[LSApplicationWorkspaceObserver applicationInstallsDidResume:]
0000000183064e60 t -[LSApplicationWorkspaceObserver applicationInstallsDidCancel:]
0000000183064f9c t -[LSApplicationWorkspaceObserver applicationInstallsDidPrioritize:]
00000001830650d8 t -[LSApplicationWorkspaceObserver applicationStateDidChange:]
0000000183065214 t -[LSApplicationWorkspaceObserver networkUsageChanged:]
0000000183065368 t -[LSApplicationWorkspaceObserver uuid]
0000000183065378 t -[LSApplicationWorkspaceObserver setUuid:]
#endif
}