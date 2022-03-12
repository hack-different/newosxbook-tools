#include <CoreFoundation/CoreFoundation.h>


// Power Mgmt Stuff 


// from IOKitUser-755.18.10/ps.subproj/IOPowerSources.h    
CFTypeRef IOPSCopyPowerSourcesInfo(void);
CFArrayRef IOPSCopyPowerSourcesList(CFTypeRef blob);
CFDictionaryRef IOPSGetPowerSourceDescription(CFTypeRef blob, CFTypeRef ps);

void dumpDict (CFDictionaryRef Dict)
{

    CFDataRef xml = CFPropertyListCreateXMLData(kCFAllocatorDefault, (CFPropertyListRef)Dict);
    
    if (xml) { 
        write(1, CFDataGetBytePtr(xml), CFDataGetLength(xml)); 
        CFRelease(xml); 
    }
}

char *getPowerDetails(int Debug) {

    CFTypeRef               powerInfo;
    CFArrayRef              powerSourcesList;
    CFDictionaryRef         powerSourceInformation;

    static char             returned[80];
                        
    powerInfo = IOPSCopyPowerSourcesInfo();

    if(! powerInfo) return ("Error: IOPsCopyPowerSourcesInfo()");

    powerSourcesList = IOPSCopyPowerSourcesList(powerInfo);
    if(!powerSourcesList) {
        CFRelease(powerInfo);
        return ("Error: IOPSCopyPowerSourcesList()");
    }
}
