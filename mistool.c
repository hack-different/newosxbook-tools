#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <sys/stat.h>

typedef void *MISProfileRef;



typedef void (^block_handler_t)(MISProfileRef);

// From libmis.h - the missing (still partial) header



// libmis exports some 65 functions in 9.3.1. This shows a dozen. 
// some are accessors. But others (notably blacklist, UPP and 
// profile creation) will be included in a future version.

#define MIS_ERROR_BASE 0xe8008000


// The #1 function in the library ...
extern int MISValidateSignature (void *File, CFDictionaryRef Opts);
// which is really just a pass through to :
extern int MISValidateSignatureAndCopyInfo (CFStringRef File, CFDictionaryRef Opts, void **Info);

extern CFStringRef MISCopyErrorStringForErrorCode(int Error);

extern int MISEnumerateInstalledProvisioningProfiles (int flags,
						      block_handler_t);

extern CFStringRef MISProfileGetValue (MISProfileRef, CFStringRef Key);
extern CFStringRef MISProvisioningProfileGetUUID (MISProfileRef);

extern CFStringRef MISProvisioningProfileGetName (MISProfileRef);
extern int MISProvisioningProfileGetVersion (MISProfileRef);
extern CFDictionaryRef MISProvisioningProfileGetEntitlements (MISProfileRef);
extern CFStringRef MISProvisioningProfileGetTeamIdentifier (MISProfileRef);

extern CFArrayRef MISProvisioningProfileGetProvisionedDevices(MISProfileRef);
extern int MISProfileIsMutable(MISProfileRef);

extern int MISProvisioningProfileIsAppleInternalProfile(MISProfileRef);
extern int MISProvisioningProfileIsForLocalProvisioning(MISProfileRef);
extern int MISProvisioningProfileProvisionsAllDevices(MISProfileRef);
extern int MISProvisioningProfileGrantsEntitlement(MISProfileRef, CFStringRef ,void *);

// Validation options - actually CFStringRefs. but whatever
extern void *kMISValidationOptionRespectUppTrustAndAuthorization;
extern void *kMISValidationOptionValidateSignatureOnly;
extern void *kMISValidationOptionUniversalFileOffset;
extern void *kMISValidationOptionAllowAdHocSigning;
extern void *kMISValidationOptionOnlineAuthorization; // triggers online validation of cert



// end header part 

/**
  * MIStool - A CLI client for libmis.dylib and friends
  *
  * 04/25/2016
  *
  * By Jonathan Levin, http://www.NewOSXBook.com/
  *
  * Part of the free sources accompanying "Mac OS X and *OS Internals"
  *
  * Free to use and abuse, but give credit where credit is due,
  * instead of just copying into GitHub and claiming ownership of
  * code you didn't really took the effort to either reverse or rebuild.
  *
  * To compile: gcc-arm64 mistool.c -o mistool -lmis -framework CoreFoundation -Wall; 
  *             jtool --sign --inplace mistool;
  *
  * Then scp to your device. Supports "list", "validate" and "errors" command line actions.
  *
  * At this point, allows you to enumerate/dump profiles. I'd add Insert/Remove, but
  * insertion requires a valid dev profile to work with - and I got none.
  *
  * Expect the reversed source of libmis.dylib (and its nefarious hench-daemons,
  * misagent and online-auth-agent) around the same time as MOX*I II - and that's soon -
  * I promise. This should give you an idea of just how thorough I'm going to get.
  *
  */

// Utility function 
char *
dumpCFArray(CFArrayRef Arr)
{
	   CFIndex count = CFArrayGetCount(Arr);
	   int v = 0;
	   const void *val;

	   static char entVal [8192];

	   entVal [0] = '\0';
	   sprintf (entVal , "(%ld values) ", count);
	   for (v = 0; v < count; v++)
	   {
	      val = CFArrayGetValueAtIndex(Arr, v);
	      // Assuming string here..
	      const char *printed = CFStringGetCStringPtr (val, // CFStringRef theString,
				    kCFStringEncodingUTF8); // CFStringEncoding encoding );

	      // yeah, possible buffer overflow here on malicious profile :-)
	      strcat (entVal, printed);
	      if (v < count -1) strcat(entVal,",");
	   }

	return (entVal);
} 

// Utility function
void 
printDictKey (const void* key, const void* value, void* context) {

	 // You could also use CFShow() here, with the dup2() trick
	 // to get CFShow to print to stdout..

	 int indent = (int) context;

	 const char *entName = CFStringGetCStringPtr (key , // CFStringRef theString,
				kCFStringEncodingUTF8); // CFStringEncoding encoding ); 

	 const char *entValue = "?";

	 // Why AAPL doesn't have constants for the typeID (so you could switch) still
	 // eludes me.
	 
	 if (CFGetTypeID(value) == CFStringGetTypeID())
		{
		   entValue = CFStringGetCStringPtr (value , // CFStringRef theString,
                                kCFStringEncodingUTF8); // CFStringEncoding encoding );
		}
	 else if (CFGetTypeID(value) == CFBooleanGetTypeID())
		{
		   entValue = CFBooleanGetValue(value) ? "true" : "false";
		}
	 else if (CFGetTypeID(value) == CFArrayGetTypeID())
		{
		 	entValue =  dumpCFArray(value);
		}

	 printf("\t%s: %s\n", entName,entValue);
}

void 
dumpProfile(MISProfileRef Prof)
{
		 CFStringRef cfsProfName= MISProvisioningProfileGetName(Prof);
		
		 const char *profName = CFStringGetCStringPtr (cfsProfName , // CFStringRef theString,
					kCFStringEncodingUTF8); // CFStringEncoding encoding ); 


		 CFStringRef cfsUuid = MISProvisioningProfileGetUUID(Prof);
		 const char *uuid = CFStringGetCStringPtr (cfsUuid , // CFStringRef theString,
					kCFStringEncodingUTF8); // CFStringEncoding encoding ); 


		 CFDictionaryRef dictEnts = MISProvisioningProfileGetEntitlements(Prof);

		 CFStringRef cfsTeamID = MISProvisioningProfileGetTeamIdentifier(Prof);
		 const char *teamID = CFStringGetCStringPtr (cfsTeamID , // CFStringRef theString,
                                        kCFStringEncodingUTF8); // CFStringEncoding encoding );



		 CFArrayRef arrayDevs = MISProvisioningProfileGetProvisionedDevices(Prof);

		 int profVersion = MISProvisioningProfileGetVersion(Prof);
		 int mutable = MISProfileIsMutable(Prof);

		 // TeamName is apparently not accessible from MISProvisioningProfile* APIs, 
		 // so use MISProfileGetValue(...) instead. For a list of values, q.v. MOX*I
		 // Vol. III, Chapter 6 (AMFI), Table 6-5.
		 
		 CFStringRef cfsTeamName = MISProfileGetValue(Prof, CFSTR("TeamName"));
		 const char *teamName = CFStringGetCStringPtr (cfsTeamName , // CFStringRef theString,
                                        kCFStringEncodingUTF8); // CFStringEncoding encoding );

		 int provisionsAll = MISProvisioningProfileProvisionsAllDevices(Prof);
		 int localProvision = MISProvisioningProfileIsForLocalProvisioning(Prof);
		 int appleInternal = MISProvisioningProfileIsAppleInternalProfile(Prof);

		 // There is also a MISProvisioningProfileGrantsEntitlement,
		 // which takes the profile and an entitlement (CFStringRef), and tells
		 // you if it's in the entitlement array.

		 // Dump out
		 printf("Profile Name: %s\n", profName);
		 printf("Version: %d\n", profVersion);
		 printf("AppleInternal: %s\n", (appleInternal ? "yes" : "no")); // I wish, but no :-(
		 printf("Local Provisioning: %s\n", (localProvision ? "yes" : "no"));
		 printf("Mutable: %s\n", (mutable ? "yes" : "no"));
		 printf("UUID: %s\n", uuid);
		 printf("Team ID: %s\n", teamID);
		 printf("Team Name: %s\n", teamName); 
		 printf("Entitlements:\n");
		 CFDictionaryApplyFunction(dictEnts, printDictKey, NULL);
		
		 if (provisionsAll) {	printf("Devices: all (Enterprise)\n"); }
		 else { printf("Device list:\n");
			printf("\t%s\n",dumpCFArray( arrayDevs));
		
		      }

}; // dumpProf


int 
main (int argc, char **argv)
{

	if (argc < 2)
	{
		fprintf(stderr,"%s list - to list all installed profiles\n", getprogname());
		fprintf(stderr,"%s errors - to list all error codes\n", getprogname());
		fprintf(stderr,"%s validate _path_ - Validate a file signature (\"what would AMFI do?\")\n", getprogname());

		exit(1);

	}

	if (strcmp(argv[1], "validate") == 0)
	{
	    if (argc != 3) { 
		 fprintf(stderr, "Work with me here.. I need a path to validate!\n");
		 exit(1);}


		 // MVS is stupid and returns e8008001 ("An unknown error has occurred")
		 // when passed a null file name. Spare it the shame and check existence first.
		 struct stat stbuf;
		 if (stat (argv[2], &stbuf)) {
		  fprintf(stderr,"File %s not found! Why are you wasting my time, little man?!\n", argv[2]);
		  exit(5);
		  }

	    void * copiedInfo =  NULL;
	    CFMutableDictionaryRef optionsDict = 
		CFDictionaryCreateMutable(kCFAllocatorDefault, // CFAllocatorRef allocator, 
				   0,			// CFIndex capacity
				   &kCFTypeDictionaryKeyCallBacks, // const CFDictionaryKeyCallBacks *keyCallBacks,
				   &kCFTypeDictionaryValueCallBacks); // const CFDictionaryValueCallBacks *valueCallBacks );



	    // Now, here's what AMFI really would do:

#ifdef IOS_9
	    // In 9.x:
	    CFDictionarySetValue(optionsDict,kMISValidationOptionRespectUppTrustAndAuthorization, kCFBooleanTrue);
#endif
	  //  CFDictionarySetValue(optionsDict,kMISValidationOptionValidateSignatureOnly,kCFBooleanTrue);
	  //  CFDictionarySetValue(optionsDict,kMISValidationOptionExpectedCDHash,CFData of CDhash here..);
	  //  CFDictionarySetValue(optionsDict,kMISValidationOptionUniversalFileOffset, CFNumber...);
	

	  // Me, I just try the ad-hoc validation, or defaults, which validates App store too.
	     CFDictionarySetValue(optionsDict,kMISValidationOptionAllowAdHocSigning, kCFBooleanTrue);
	  
		
	    // $%#$%$#%# CFStrings

	    CFStringRef FileName =
             CFStringCreateWithCStringNoCopy (kCFAllocatorDefault, // CFAllocatorRef alloc, 
			 argv[2], // const char *cStr, 
			 kCFStringEncodingUTF8, // CFStringEncoding encoding, 
			 kCFAllocatorDefault); // CFAllocatorRef contentsDeallocator );

	    // Using MISValidateSignatureAndCopyInfo because on JB devices MISValidateSignature
	    // is re-exported to return 0 in any case...
	    // change this to MVS if you want to check if your code signature bypass works:

	    int rc = MISValidateSignatureAndCopyInfo(FileName, optionsDict,NULL);
	    if (rc)
		{
		     fprintf(stderr,"Error %d (0x%x) - ",rc, rc);
		      CFShow(MISCopyErrorStringForErrorCode(rc));
		}
	    else {printf("Valid!\n"); }

	    return (rc);
	}

	if (strcmp(argv[1], "list") == 0)
	{
	block_handler_t	miscbb = 
		 ^(MISProfileRef prof) {
		 // Stupid Blocks.
		 dumpProfile (prof);
		};

	int rc = MISEnumerateInstalledProvisioningProfiles (0, miscbb);

	if (rc !=0) { fprintf(stderr,"Err.. Something's not right?\n");
		      // CFShow prints to stderr anyway, so why not?
		      CFShow(MISCopyErrorStringForErrorCode(rc));
		    }
	
		exit(rc);
	} // for list
	

	if (strcmp(argv[1], "errors") == 0)
	{
	int i =0;
	for (i = MIS_ERROR_BASE;
	     i < MIS_ERROR_BASE + 30;  // CopyErrorString handles 29, just to be safe, I +1
	     i++)
	{
		fprintf(stderr,"Error 0x%x: ", i);
		CFStringRef err =MISCopyErrorStringForErrorCode(i);
		if (err) { CFShow(err);}

	}
	

		exit(0);
	} // for error


	fprintf(stderr,"*Sigh*\n");

	exit(1);

}
