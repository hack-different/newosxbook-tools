#include <stdio.h>
#include <mach/mach.h>
#define IOKIT	// to unlock device/device_types..
#include <device/device_types.h> // for io_name, io_string
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



void listProps(io_service_t	Service)
{

  
  CFMutableDictionaryRef propertiesDict;



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

  printf ("KR: %d\n", kr);
}
int main(int argc, char **argv)
{
    io_iterator_t deviceList;
    io_service_t  device;
    io_name_t     deviceName;
    io_string_t   devicePath;
    char	 *ioPlaneName = "IOService";
    int 	  dev = 0;

    kern_return_t kr;

    if (argv[1]) ioPlaneName = argv[1];
	printf("So far..\n");

    // Iterate over all services matching user provided class.
    // Note the call to IOServiceMatching, to create the dictionary

    kr = IOServiceGetMatchingServices(kIOMasterPortDefault,
                                     IOServiceMatching("IOService"),
                                     &deviceList);
    if (kr)
    {
          fprintf(stderr,"IOServiceGetMatchingServices: error\n");
          exit(1);
        }
    if (!deviceList) {  fprintf(stderr,"No devices matched\n"); exit(2); }

	printf("So far..\n");
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
         printf("%s\t%s\n",deviceName, devicePath);
	 listProps(device);
    }

    if (device) {
         fprintf (stderr,
          "Iterator invalidated while getting devices. Did hardware configuration change?\n");
        }
    return kr;
}
