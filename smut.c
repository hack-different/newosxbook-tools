/*
 * SM(BIOS) UTility
 *
 * One of the source code examples from MOXiI 2, Volume II
 *
 * Dumps the AppleSMBIOS property from the IORegistry in a nice and
 * more human readable format.
 *
 *
 * To compile:
 *  	gcc smut.c -o bios -framework IOKit -framework CoreFoundation
 *
 * (will compile <del>cleanly</del> just fine, maybe a warning or two.. I promise)
 *
 * License: Free to use. But don't forget to give credit where due if you
 *          found this useful.
 *
 * (This could actually work on any Intel device with SMBIOS - feel free
 *  to port this to Linux, if there isn't have something for that already)
 *
 */

#include <stdio.h>
#include <mach/mach.h>

typedef unsigned char  UInt8 ;
typedef unsigned short UInt16 ;
typedef unsigned int  UInt32 ;
typedef unsigned long long  UInt64 ;


const char ver[] =
	{"@(#) PROGRAM: smut 1.0 \tPROJECT: Jtools"}; // for what(1)

#include "SMBIOS.h" // updated from Apple's SMBIOS project (q.v. http://newosxbook.com/code/listings/SMBIOS.h)

#define IOKIT	// to unlock device/device_types..
#include <device/device_types.h> // for io_name, io_string
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>


// from IOKit/IOKitLib.h
extern const mach_port_t kIOMasterPortDefault;

// from IOKit/IOTypes.h
//typedef mach_port_t     io_object_t;
typedef io_object_t     io_connect_t;
typedef io_object_t     io_enumerator_t;
typedef io_object_t     io_iterator_t;
typedef io_object_t     io_registry_entry_t;
typedef io_object_t     io_service_t;


kern_return_t
IOServiceGetMatchingServices(
        mach_port_t     masterPort,
        CFDictionaryRef matching,
        io_iterator_t * existing );

CFMutableDictionaryRef
IOServiceMatching(
        const char *    name );


// typedef unsigned long IOOptionBits;
kern_return_t
IORegistryEntryCreateCFProperties(
        io_registry_entry_t     entry,
        CFMutableDictionaryRef * properties,
        CFAllocatorRef          allocator,
        IOOptionBits            options );


CFTypeRef
IORegistryEntryCreateCFProperty(
        io_registry_entry_t     entry,
        CFStringRef             key,
        CFAllocatorRef          allocator,
        IOOptionBits            options );


// Globals go here:
int g_verbose = 0;
#define vprintf if (g_verbose) printf


int hexDump (unsigned char *Buf, int Len)
{
        int i = 0 ;

        fprintf (stdout, "0x%04X: ", 0);
        for (i = 0 ; i < Len; i ++) {
                if (i && (i % 8 == 0)) {
                        fprintf(stdout,"\t%c%c%c%c%c%c%c%c\n",
                        isprint(Buf[i-8]) ? Buf[i-8] : '.',
                        isprint(Buf[i-7]) ? Buf[i-7] : '.',
                        isprint(Buf[i-6]) ? Buf[i-6] : '.',
                        isprint(Buf[i-5]) ? Buf[i-5] : '.',
                        isprint(Buf[i-4]) ? Buf[i-4] : '.',
                        isprint(Buf[i-3]) ? Buf[i-3] : '.',
                        isprint(Buf[i-2]) ? Buf[i-2] : '.',
                        isprint(Buf[i-1]) ? Buf[i-1] : '.');
                        
                        fprintf (stdout, "0x%4X: ", i);
                }
                fprintf(stdout, "0x%02X ", (unsigned char) Buf[i]);
        }

        fprintf(stdout,"\n");
        return 0;
} // hexDump


CFDataRef getProp(io_service_t	Service, char *IOPropertyName)
{

  
  CFMutableDictionaryRef propertiesDict;


  if (IOPropertyName)
	{
  		CFStringRef	cfStrPropertyName = CFStringCreateWithCString (kCFAllocatorDefault, // CFAllocatorRef alloc,
						    IOPropertyName, // const char *cStr, 
						    kCFStringEncodingASCII); // CFStringEncoding encoding ); 

		CFTypeRef  propertyRef = IORegistryEntryCreateCFProperty(Service, // io_registry_entry_t     entry,
							  cfStrPropertyName, // CFStringRef	key,
							  kCFAllocatorDefault, // CFAllocatorRef allocator,
							  0);  // IOOptionBits options
						

		
		if (!propertyRef)
			{
				printf("Property %s not found\n", IOPropertyName);
				return NULL;
			}

		if (CFGetTypeID(propertyRef) == CFStringGetTypeID()) {
			   int len = CFStringGetLength(propertyRef);
			   printf("LEN IS %d\n", len);
			   const char* str = CFStringGetCStringPtr(propertyRef, kCFStringEncodingASCII);
			   int i = 0;
			   for (i = 0 ; i < len; i ++) putchar (str[i]);
			   printf("\n");
			}
		if (CFGetTypeID(propertyRef) == CFDataGetTypeID()) {
			   return (propertyRef);

			   

		}

	}

   else {
  kern_return_t kr = IORegistryEntryCreateCFProperties( Service,
                                                    &propertiesDict,
                                                    kCFAllocatorDefault,
                                                    kNilOptions );

    CFDataRef xml = CFPropertyListCreateXMLData(kCFAllocatorDefault,
                                                (CFPropertyListRef)propertiesDict);
    if (xml) {
        write(1, CFDataGetBytePtr(xml), CFDataGetLength(xml));
        CFRelease(xml);
	}

	} // end else
	return 0;
}

char *getSMBString (unsigned char *SMBElement,
	            unsigned int  stringNum)
{

  struct SMBStructHeader *sh = (struct SMBStructHeader *) SMBElement;
  int s = 1;
  char *theString = (char *)(SMBElement + sh->length);
  int off = 0 ;
  
  while (s < stringNum)
	{
	   while (theString[0])  { theString++;}
	   // Now points to NULL, so advance.
	   theString++;
	   s++;;
	}

  if (theString[0]) {return (theString);} // If this is the n-th string
  else return( "");
  
  

}

const char *bootStatusToText (int Use) {
	switch (Use){

	case 0: return "no errors detected";
	case 1: return "no bootable media";
	case 2: return "normal OS failed to load";
	case 3: return "firmware detected failure";
	case 4: return "OS detected failure";
	case 5: return "User requested boot";
	case 6: return "system security violation";
	case 7: return "previously requested image";
	case 8: return "watchdog timer expiration";
	default: return "unknown/reserved";

	}
} // bootStatusToText
const char *memLocToText (int Use) {
	switch (Use){

	case 1: return "other";
	case 2: return "unnknown";
	case 3: return "system board/motherboard";
	case 4: return "ISA add-on";
	case 5: return "EISA add-on";
	case 6: return "PCI add-on";
	case 7: return "MCA add-on";
	case 8: return "PCMCIA add-on";
	case 9: return "proprietary add-on";
	default: return "?!";
	}
} // memLoc

const char *memUseToText (int Use) {
	switch (Use){

	case 1: return "other";
	case 2: return "unnknown";
	case 3: return "system memory";
	case 4: return "video memory";
	case 5: return "flash memory";
	case 6: return "NVRAM";
	case 7: return "cache memory";
	default: return "?!";

	}

} // memUs

const char *enclosureTypeToText (int Type){
	switch (Type)
	{
	case 1: return "other";
	case 2: return "unnknown";
	case 3: return "dekstop";
	case 4: return "low profile desktop";
	case 5: return "pizza box";
	case 6: return "mini tower";
	case 7: return "tower";
	case 8: return "portable";
	case 9: return "laptop";
	case 10: return "notebook";
	case 11: return "handheld";
	case 12: return "docking station";
	case 13: return "all in one";
	case 14: return "sub notebook";
	default: return "Too many codes - J hasn't filled all";
	} // 

} // deviceTypeToText
const char *deviceTypeToText (int Type){
	switch (Type)
	{
	case 1: return "other";
	case 2: return "unnknown";
	case 3: return "video";
	case 4: return "SCSI";
	case 5: return "Ethernet";
	case 6: return "Token Ring...";
	case 7: return "Sound";
	case 8: return "PATA";
	case 9: return "SATA";
	case 10: return "SAS";
	default: return "?!";
	} // 

} // deviceTypeToText

ssize_t	parseSMBIOSStruct(unsigned char *Bytes, int Offset, int Len)
{
	// An SMBIOSStruct starts with a header, defining its type and length:
        // struct SMBStructHeader {
    	//	SMBByte    type;
   	//      SMBByte    length;
   	//	SMBWord    handle;
	// };

	
	struct SMBStructHeader *ssh = (struct SMBStructHeader *)(Bytes +Offset);

	vprintf("%d: Header for type %d, Length %u bytes, handle %hd\n", 
		 Offset,
		 Bytes[Offset],
	  	 Bytes[Offset+1],
		(*(SMBWord *) (Bytes + Offset + 2)));
		

	// Advancing however many bytes are specified in the length field will
	// bring us to the beginning of the string set - or the first of the 
	// double-NULL byte terminator

	unsigned char *current = Bytes + Offset;
        SMBByte len = ssh->length; //current[1]; // Can also get this via struct.

	printf("0x%x:", Offset);
	switch (Bytes[Offset]) // which is the type
	{
		case  kSMBTypeBIOSInformation:            // =  0,
		   {
#if 0
struct SMBBIOSInformation {
    SMB_STRUCT_HEADER               // Type 0
    SMBString  vendor;              // BIOS vendor name
    SMBString  version;             // BIOS version
    SMBWord    startSegment;        // BIOS segment start
    SMBString  releaseDate;         // BIOS release date
    SMBByte    romSize;             // (n); 64K * (n+1) bytes
    SMBQWord   characteristics;     // supported BIOS functions
};


#endif
		    printf("BIOS Information:\n");
		  struct SMBBIOSInformation *bi = (struct SMBBIOSInformation *) current;
			printf("\tVendor: %s\n",  getSMBString((void *) current, bi->vendor));
			printf("\tVersion: %s\n",  getSMBString((void *) current, bi->version));
			printf("\tRelease Date: %s\n",  getSMBString((void *) current, bi->releaseDate));


			}
		
		    break;
	
		case  kSMBTypeSystemInformation:          // =   1,
		    printf("System Information:\n");
		    
		    struct SMBSystemInformation *si = (struct SMBSystemInformation *) current;
		    printf("\tManufacturer: %s\n", getSMBString (current, si->manufacturer));
		    printf("\tProduct Name: %s\n", getSMBString (current, si->productName));
		    printf("\tVersion: %s\n", getSMBString (current, si->version));
		    printf("\tSerial #: %s\n", getSMBString (current, si->serialNumber));
		    
		    break;
	
		case  kSMBTypeBaseBoard:                  // =   2,
		    printf("Base Board:\n");

		{ 
			struct SMBBaseBoard *bb= (struct SMBBaseBoard *) current;
			
			printf("\tManufacturer: %s\n", getSMBString((void *) current, bb->manufacturer));
			printf("\tProduct: %s\n", getSMBString((void *) current, bb->product));
			printf("\tVersion: %s\n", getSMBString((void *) current, bb->version));
			printf("\tSerial #: %s\n", getSMBString((void *) current, bb->serialNumber));
			printf("\tLocation: %s\n", getSMBString((void *) current, bb->locationInChassis));

		}


		    break;
		case  kSMBTypeSystemEnclosure:            // =   3,
		   {
		    printf("System Enclosure:\n");
		struct SMBSystemEnclosure *se = (struct SMBSystemEnclosure *) current;
		printf("\tManufacturer: %s\n", getSMBString((void *)current, se->manufacturer));
		printf("\tVersion: %s\n", getSMBString((void*)current, se->version));
		printf("\tSerial #: %s\n", getSMBString((void*)current, se->serialNumber));
		printf("\tType: %s\n", enclosureTypeToText(se->type & 0x7f));

#if 0
struct SMBSystemEnclosure {
    SMB_STRUCT_HEADER               // Type 3
    SMBString  manufacturer;
    SMBByte    type;
    SMBString  version;
    SMBString  serialNumber;
    SMBString  assetTagNumber;
    SMBByte    bootupState;
    SMBByte    powerSupplyState;
    SMBByte    thermalState;
    SMBByte    securityStatus;
    SMBDWord   oemDefined;

#endif
		}
		    break;
		case  kSMBTypeProcessorInformation:       // =   4,
		    printf("Processor Information:\n");
	
		    struct SMBProcessorInformation *pi = (struct SMBProcessorInformation *) (Bytes + Offset);
	
		    printf("\tSocket designation: %s\n", getSMBString(Bytes + Offset,
								      pi->socketDesignation));
		    printf("\tManufacturer: %s\n", getSMBString(Bytes + Offset, pi->manufacturer));
		    printf("\tVersion: %s\n", getSMBString(Bytes + Offset, pi->processorVersion));
		    printf("\tSerial #: %s\n", getSMBString(Bytes + Offset, pi->serialNumber));
		    printf("\tClocks: External %f Ghz\tMaximum: %f Ghz\tCurrent: %f Ghz\n",
			    ((float)pi->externalClock / 1000),
			    ((float)pi->maximumClock / 1000),
			    ((float)pi->currentClock / 1000));


		    printf("\tCaches: L1: %d\t L2: %d\tL3: %d\n",
			  pi->L1CacheHandle,
			  pi->L2CacheHandle,
			  pi->L3CacheHandle);

#if 0
struct SMBProcessorInformation {
    // 2.0+ spec (26 bytes)
    SMB_STRUCT_HEADER               // Type 4
    SMBString  socketDesignation;
    SMBByte    processorType;       // CPU = 3
    SMBByte    processorFamily;     // processor family enum
    SMBString  manufacturer;
    SMBQWord   processorID;         // based on CPUID
    SMBString  processorVersion;
    SMBByte    voltage;             // bit7 cleared indicate legacy mode
    SMBWord    externalClock;       // external clock in MHz
    SMBWord    maximumClock;        // max internal clock in MHz
    SMBWord    currentClock;        // current internal clock in MHz
    SMBByte    status;
    SMBByte    processorUpgrade;    // processor upgrade enum
    // 2.1+ spec (32 bytes)
    SMBWord    L1CacheHandle;
    SMBWord    L2CacheHandle;
    SMBWord    L3CacheHandle;
    // 2.3+ spec (35 bytes)
    SMBString  serialNumber;
    SMBString  assetTag;
    SMBString  partNumber;
};
#endif


		    break;
		case  kSMBTypeMemoryModule:               // =   6,
		    printf("Memory Module:\n");
		    break;
		case  kSMBTypeCacheInformation:           // =   7,
			
			{
			struct SMBCacheInformation *ci = (struct SMBCacheInformation *) current;
		    printf("Cache Information (#%d):\n", ci->header.handle);
		    printf("\tSocket designation: %s\n", getSMBString((unsigned char *)ci, ci->socketDesignation));
		    printf("\tSize: %d/%dKB\n", ci->installedSize, ci->maximumCacheSize);
		if (ci->cacheSpeed) { printf("\tSpeed: %d\n", ci->cacheSpeed);}
		
#if 0

struct SMBCacheInformation {
    SMB_STRUCT_HEADER               // Type 7
    SMBString  socketDesignation;
    SMBWord    cacheConfiguration;
    SMBWord    maximumCacheSize;
    SMBWord    installedSize;
    SMBWord    supportedSRAMType;
    SMBWord    currentSRAMType;
    SMBByte    cacheSpeed;
    SMBByte    errorCorrectionType;
    SMBByte    systemCacheType;
    SMBByte    associativity;
};
#endif

		}
		    break;
		case  kSMBTypeSystemSlot:                 // =   9,
		    printf("System Slot:\n");
		
		    struct SMBSystemSlot *ss = (struct SMBSystemSlot *) current;
		    printf("\tDesignation: %s\n", getSMBString(current, ss->slotDesignation));

#if 0
struct SMBSystemSlot {
    // 2.0+ spec (12 bytes)
    SMB_STRUCT_HEADER               // Type 9
    SMBString   slotDesignation;
    SMBByte     slotType;
    SMBByte     slotDataBusWidth;
    SMBByte     currentUsage;
    SMBByte     slotLength;
    SMBWord     slotID;
    SMBByte     slotCharacteristics1;
    // 2.1+ spec (13 bytes)
    SMBByte     slotCharacteristics2;
};
#endif

		    break;
		case  kSMBTypePhysicalMemoryArray:        // =  16,
			{
		    printf("Physical Memory Array:\n");

#if 0
struct SMBPhysicalMemoryArray {
    // 2.1+ spec (15 bytes)
    SMB_STRUCT_HEADER               // Type 16
    SMBByte    physicalLocation;    // physical location
    SMBByte    arrayUse;            // the use for the memory array
    SMBByte    errorCorrection;     // error correction/detection method
    SMBDWord   maximumCapacity;     // maximum memory capacity in kilobytes
    SMBWord    errorHandle;         // handle of a previously detected error
    SMBWord    numMemoryDevices;    // number of memory slots or sockets
};

#endif
			struct SMBPhysicalMemoryArray * pma =  (struct SMBPhysicalMemoryArray *) current;

			printf("\tPhysical Location: %d (%s)\n", pma->physicalLocation, memLocToText( pma->physicalLocation ));
			printf("\tArray Use: %d (%s)\n", pma->arrayUse, memUseToText(pma->arrayUse));
			printf("\tMaximum Capacity: %dGB\n", pma->maximumCapacity/ (1024*1024));

			}
		    break;

		case  kSMBTypeMemoryDevice:               // =  17,
		    printf("Memory Device:\n");
		    struct SMBMemoryDevice *md = (struct SMBMemoryDevice *) (Bytes + Offset);
		
		    printf("\tSize: %d %s\tSpeed: %d Mhz",
			    md->memorySize & 0x7FFF,
			    (md->memorySize & 0x8000 ? "KB" : "MB"),
			    md->memorySpeed);

		    printf("\tDevice Locator: %s\tBank Locator: %s\n",	
			   getSMBString (Bytes + Offset, md->deviceLocator),
			   getSMBString (Bytes + Offset, md->bankLocator));
		    printf("\tManufacturer: %s\tAsset Tag: %s\n",
			   getSMBString (Bytes + Offset, md->manufacturer),
			   getSMBString (Bytes + Offset, md->assetTag));
		    printf("\tSerial #: %s\tPart #: %s\n",
			   getSMBString (Bytes + Offset, md->serialNumber),
			   getSMBString (Bytes + Offset, md->partNumber));
	
		

#if 0
struct SMBMemoryDevice {
    // 2.1+ spec (21 bytes)
    SMB_STRUCT_HEADER               // Type 17
    SMBWord    arrayHandle;         // handle of the parent memory array
    SMBWord    errorHandle;         // handle of a previously detected error
    SMBWord    totalWidth;          // total width in bits; including ECC bits
    SMBWord    dataWidth;           // data width in bits
    SMBWord    memorySize;          // bit15 is scale, 0 = MB, 1 = KB
    SMBByte    formFactor;          // memory device form factor
    SMBByte    deviceSet;           // parent set of identical memory devices
    SMBString  deviceLocator;       // labeled socket; e.g. "SIMM 3"
    SMBString  bankLocator;         // labeled bank; e.g. "Bank 0" or "A"
    SMBByte    memoryType;          // type of memory
    SMBWord    memoryTypeDetail;    // additional detail on memory type
    // 2.3+ spec (27 bytes)
    SMBWord    memorySpeed;         // speed of device in MHz (0 for unknown)
    SMBString  manufacturer;
    SMBString  serialNumber;
    SMBString  assetTag;
    SMBString  partNumber;
};
#endif

		    break;
		case  kSMBType32BitMemoryErrorInfo:       // =  18,
		    printf("32-Bit Memory Error Info:\n");
		    break;
		case jSMBTypePortableBatteryInfo:	// == 22
			{
		     printf("Portable Battery Info:\n");
		struct SMBPortableBatteryInformation  *bi = (struct SMBPortableBatteryInformation *) current;

		    printf("\tManufacturer: %s %d\n", getSMBString (current, bi->manufacturer), si->manufacturer);
		    printf("\tSerial #: %s\n", getSMBString (current, bi->serialNumber));
		    printf("\tName: %s\n", getSMBString (current, bi->deviceName));
		    printf("\tDesign Capacity: %d\n",bi->designCapacity);
		    printf("\tDesign Voltage: %d\n",bi->designVoltage);
			}
		    break;

		case jSMBTypeManagementDeviceComponentInfo:	// == 35
		     printf("Management Device Component Info:\n");
		    break;

		case jSMBTypeManagementDeviceThresholdData:	// == 36
		     printf("Management Device Threshold Data:\n");
		    break;

		case jSMBTypeOnboardDevicesExtendedInfo:	// = 41
		     printf("Onboard Devices Extended Info:\n");
			{
		struct SMBOnboardDevicesExtendedInformation  *odei = (struct SMBOnboardDevicesExtendedInformation *)				current;
			printf("\tReference Designation : %s\tType: %s (%s)\n",
				getSMBString(current, odei->referenceDesignation), 
				deviceTypeToText(odei->deviceType & 0x7f),
				(odei->deviceType &0x80 ? "enabled" : "disabled"));

#if 0
struct SMBOnboardDevicesExtendedInformation {
        SMB_STRUCT_HEADER       
        SMBByte referenceDesignation;
        SMBByte deviceType;
        SMBByte deviceTypeInstance;
        SMBWord SegmentGroupNumber;
        SMBByte busNumber;
        SMBByte bitField;
};
#endif
			}
			
			break;

		case jSMBTypeGroupAssociations: // 14
		     printf("Group Associations\n");
		     break;
		case  jSMBTypeIMIDeviceInfo:	//  38
			printf("IPMI Device Information\n");
			break;

		case  kSMBType64BitMemoryErrorInfo:       // =  33,
		    printf("64-Bit Memory Error Info:\n");
		    break;

	
		// J's Additions:
		case 8: // Port Connector Information: (7.9)
		    printf("Port Connector Information\n");
		    struct SMBPortConnectorInformation *pci =
			(struct SMBPortConnectorInformation *) (Bytes + Offset);
		    
		    printf("\tInternal Reference Designator: %s\tType: %d\n",
			   getSMBString(Bytes + Offset , pci->InternalReferenceDesignator),
			   pci->InternalConnectorType);

		    printf("\tExternal Reference Designator: %s\tType: %d\n",
			   getSMBString(Bytes + Offset , pci->ExternalReferenceDesignator),
			   pci->ExternalConnectorType);


		    break;
		case 10: // On Board Device Information (obsolete in 2.7)
		    // This is obsolete, but also different in that the
		    // length is longer than the average

		   {
		    int numDevices = (current[1] -4) /2;
		    printf("On Board Device Information (%d Devices):\n", numDevices);
		    int d = 0;
		    for (d = 1 ; d <= numDevices;  d++)
			{
			   printf("\t%d: %s\n",
				  d, getSMBString (current, 
						   *(current + 5 + 2 * (d-1)))); 
			}
		   
		   
			}
		    break;
		case 11: // OEM Strings
		    printf("OEM Strings: (%d Strings)\n", current[0x4]);
		    Offset += 5;
		    int s = 0;

		    while (Bytes[Offset])
			{
				printf("\tString %d: ", s);
				while (Bytes[Offset]) {
					if ((Bytes[Offset] == ' ') || ispunct (Bytes[Offset]) || (isalnum(Bytes[Offset]))) {
						putchar (Bytes[Offset]);
						}
					else 
						{ 
						   switch (Bytes[Offset])
							{ 
							case 0xa: printf("\\r"); break;
							case 0xd: printf("\\n"); break;

							default:
							printf("\\x%02x", Bytes[Offset]);
							} // end switch (Bytes[Offset]) for case 11
						}
					Offset++;
					} ; // end while
			  	printf("\n");
				s++;
				Offset++;

			}
		 
		    
		    return (Offset+1);
		    break;
		case 12: // System Configuration Options
		    printf("System Configuration Options:\n");
		    
		    break;

		case 13: // BIOS Language Information
	            printf("BIOS Language Information:\n");
		    printf("Installable Languages: %d\n",
				*(current + 0x4));
		    printf("String: %d %s\n", *(current + 0x15), 
					getSMBString (current ,
						*(current+ 0x15)));
		
		    break;

		case jSMBTypeMemoryArrayMappedAddress: // Memory Array Mapped Address
		
		    printf("Memory Array Mapped Address:\n");
	{
		struct SMBMemoryArrayMappedAddress *mama = (struct SMBMemoryArrayMappedAddress *) current;

		printf("\tAddress range: 0x%x-0x%x\n",
			mama->startingAddress,
			mama->endingAddress);




	}
		    break;

		case jSMBTypeSystemBootInfo:	// == 32

		    printf("System Boot Information:\n");
			{
		    struct SMBSystemBootInformation *sbi = (struct SMBSystemBootInfomration *) current;
		
		    int i =0;
		    for (i = 0 ; i < sbi->header.length - 10; i++){


				printf("\tStatus[%d]: %d (%s)\n", i,sbi->BootStatus[i],
					bootStatusToText(sbi->BootStatus[i]));


			}

		    }

		    break;

		case 127: // End of table
			printf("End of Table\n");
			break;;
		/* Apple Specific Structures */
		case  kSMBTypeFirmwareVolume:             // =  128,
		    printf("Firmware Volume:\n");
		    break;
		case  kSMBTypeMemorySPD:                  // =  130,
		    printf("Memory SPD:\n");
			hexDump((void *)current, ssh->length);
		    break;
		case  kSMBTypeOemProcessorType:           // =  131,
		    printf("OEM Processor Type:\n");
		    break;
		case  kSMBTypeOemProcessorBusSpeed:       // =  132
		    printf("Firmware Volume:\n");
		    break;

		case 134: // not sure what this is yet
		    printf("Apple SMC Version (probably - 134)\n");
		 {struct SMBSMCVersion *sv = (struct SMBSMCVersion *) current;
		  printf("\tVersion String: %s\n",  sv->version);}
			hexDump((void *)current, ssh->length);
		    break;
		default:
		
		    printf("Unknown type: %d?! (%d bytes)\n", *current , ssh->length);
			hexDump((void *)current, ssh->length);

		    break;
		};



	Offset += len +1 ; // To get to strings

	if (Bytes[Offset]) Offset--; // If it's the NULL byte terminator, correct.

	while (Bytes[Offset])
	{
		while (Bytes[Offset++]) {};

	}

	// If we're here, we hit two NULLs.

	return (Offset +1 );
}

void parseBIOSData (CFDataRef  BIOSDump)
{
   int len = CFDataGetLength(BIOSDump);
   const char *bytes = alloca (len);
   CFDataGetBytes(BIOSDump, CFRangeMake(0,CFDataGetLength(BIOSDump)), (void*)bytes);
   int i = 0;
   if (g_verbose) {
	printf("Data:\n");

   for (i = 0 ; i < len; i ++)
	 { if (isalnum(bytes[i])) putchar (bytes[i]);
		else putchar ('.');
	 }

	} // end g_verbose

   // Now parse SMBios Structures
   
   for (i = 0; i < len; )
	{
		i = parseSMBIOSStruct((void *)bytes, i, len);
	}

  

}

int main(int argc, char **argv)
{
io_iterator_t deviceList;
io_service_t  device;
io_name_t     deviceName;
io_string_t   devicePath;
char	 *ioPlaneName = "IOService";
char	 *ioClassName = "AppleSMBIOS";
char	 *ioPropertyName = "SMBIOS";
int 	  dev = 0;

kern_return_t kr;

if (argc >1)
{
	if (strcmp(argv[1], "-v") == 0) { g_verbose++;}
  

}

// Iterate over all services matching AppleSMBIOS (only one)
// Note the call to IOServiceMatching, to create the dictionary

kr = IOServiceGetMatchingServices(kIOMasterPortDefault,
			     IOServiceMatching(ioClassName),
			     &deviceList);
if (kr)
{
  fprintf(stderr,"IOServiceGetMatchingServices: error\n");
  exit(1);
}
if (!deviceList) {  fprintf(stderr,"%s - No devices matched\n", ioClassName); exit(2); }

while ( IOIteratorIsValid(deviceList) &&         
    (device = IOIteratorNext(deviceList))) {

 kr = IORegistryEntryGetName(device, deviceName);
 if (kr) 
    {
	fprintf (stderr,"Error getting name for device\n"); 
	IOObjectRelease(device);
	continue;
    }

 kr = IORegistryEntryGetPath(device, ioPlaneName, devicePath);

 if (kr) { 
	// Device does not exist on this plane
	IOObjectRelease(device); 
	continue; 
	}
 

 dev++;
 vprintf("%s\t%s\n",deviceName, devicePath);
 CFDataRef	BIOSDump = getProp(device, ioPropertyName);
		 

	parseBIOSData (BIOSDump);



}

if (device) {
 fprintf (stderr,
  "Iterator invalidated while getting devices. Did hardware configuration change?\n");
}
return kr;
}
