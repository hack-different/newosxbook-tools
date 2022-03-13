#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define DEV_TREE_PROP_NAME_LEN	32
#define DEV_TREE_PROP_VALUE_LEN	8192
#define DEV_TREE_PROP_FLAGS_MASK (0xf0000000)

#define PAD32(x) (((x) + 3) & ~3) // Macro to make sure values get padded on 32-bit boundary

/**
 *  dtize.c - Another http://NewOSXBook.com/ source example
 *
 *  A simple device tree dumper. Similar to what imagine (for img3) had, but works on new
 *  Img4s as well. Doesn't actually do any DER processing (hence quick)
 *   
 *  License: ABSE. Rest of the world - Use and abuse. The day of open iOS on generic ARM64
 *           architectures drawns (infinitessimally) closer..
 *
 */



int indent = 0;


void getDTNode (int fd) {

	int rc = 0;
	char devTreePropName[DEV_TREE_PROP_NAME_LEN];
	unsigned char devTreePropValue[DEV_TREE_PROP_VALUE_LEN];
	uint32_t devTreePropLenAndFlags = 0;
	uint32_t devTreePropFlags = 0;
	uint32_t	numProperties = 0;
	uint32_t	numChildren = 0 ;

	rc = read (fd, &numProperties, sizeof(uint32_t));
	rc = read (fd, &numChildren, sizeof(uint32_t));
	
	printf("NUM PROPERTIES: %d, NUM CHILDREN: %d\n", numProperties, numChildren);
	int p = 0;

	while (p < numProperties ) {

	rc = read (fd,  devTreePropName, DEV_TREE_PROP_NAME_LEN);

	rc = read (fd, &devTreePropLenAndFlags, sizeof(uint32_t));
	
	
	devTreePropFlags = devTreePropLenAndFlags & DEV_TREE_PROP_FLAGS_MASK;

	devTreePropLenAndFlags &= ~DEV_TREE_PROP_FLAGS_MASK;
	if (devTreePropLenAndFlags > DEV_TREE_PROP_VALUE_LEN)
	{
		off_t pos = lseek (fd, 0, SEEK_CUR);
		fprintf(stderr,"Error: Excessive device tree property len Pos %llx (%d > %d)\n",
				pos,
			devTreePropLenAndFlags , DEV_TREE_PROP_VALUE_LEN);
		exit(0);
	}
	
	rc = read (fd, devTreePropValue, PAD32(devTreePropLenAndFlags));

	
	int i = 0;
	while (i < indent) { printf("---"); i++;};
	printf("%s (%d bytes): ",  devTreePropName, devTreePropLenAndFlags);
	
	static char allZeros[DEV_TREE_PROP_VALUE_LEN] = { 0 };




	if (devTreePropLenAndFlags == 4) {

		// Print as integer
		printf("%d (*)\n", *(uint32_t *) devTreePropValue);
		}
	else if (memcmp(devTreePropValue, allZeros, devTreePropLenAndFlags) ==0) {
			printf("(NULL)\n");
		}

	else { // safe string
			int j = 0;
			for (j = 0 ; j < devTreePropLenAndFlags; j++)
			{
				if (isprint(devTreePropValue[j])) putc(devTreePropValue[j], stdout);
				else if ((j == devTreePropLenAndFlags - 1)  && !devTreePropValue[j]) { /* skip terminating NULL */}
	
				else printf("\\x%02x", devTreePropValue[j]);
			
			}
			printf("\n");
		}
	p++;
	}

	indent++;
	int c = 0;
	while (c< numChildren ) {
		printf("Child %d:", c);
		
		getDTNode(fd);
		c++;
	}
	indent--;

} // end getDTNode;


int main (int argc, char **argv) {

	if (!argv[1]) {

		fprintf(stderr,"Usage: %s DeviceTree....im4p\n", argv[0]);

		exit(1);
	}
	int fd = open (argv[1],O_RDONLY);

	if (fd < 0){ perror (argv[1]); exit(2);}


	// The IMG4p header is a DER header of 48 bytes:
	// 
  	//   0:d=0  hl=5 l=168296 cons: SEQUENCE          
   	//   5:d=1  hl=2 l=   4 prim: IA5STRING         :IM4P
  	//  11:d=1  hl=2 l=   4 prim: IA5STRING         :dtre
  	//  17:d=1  hl=2 l=  29 prim: IA5STRING         :EmbeddedDeviceTrees-3480.40.4
  	
	// @TODO: Handle DER. For now, just skip to offset 19, which is where EmbeddedDevice
	// in IA5String starts
	//
	//00000000  30 83 02 91 68 16 04 49  4d 34 50 16 04 64 74 72  |0...h..IM4P..dtr|
	//00000010  65 16 1d 45 6d 62 65 64  64 65 64 44 65 76 69 63  |e..EmbeddedDevic|
	//00000020  65 54 72 65 65 73
	//
	
	char buf[256];

	int rc = read (fd, buf, 19);

	if (buf[17] != 0x16) { fprintf(stderr,"Error: Expected IA5STRING for EmbeddedDevice...\n"); exit(5); }

	// Otherwise the length of the Embedded DeviceTree.... string is in buf[18]
	
	int len = buf[18];
	 rc = read (fd,buf, len);

	// At this offset we expect the last two bytes
	// At offset 48 we expect an OCTET STRING (4)
	// 48:d=1  hl=5 l=168248 prim
	 
	char dtBegins[] = { '\x04' , '\x83' };


	rc = read (fd, buf, 2);
	if (memcmp (buf, dtBegins,2)) {

		fprintf(stderr,"Error: Expected OCTET STRING with 3 byte length at offet 48\n"); exit(3); 
	}
	

	int totalLenLen = 3;
	// Otherwise get total length, assuming 3 bytes
	uint32_t totalLen = 0;

	rc = read (fd, &totalLen, totalLenLen);

	totalLen <<=8;
	totalLen = ntohl(totalLen);
	printf("Device Tree of %u bytes\n", totalLen);

	// @TODO: Sanity check for size (vs. fstat)
	// 	
	//
	

	uint32_t	numProperties = 0;
	uint32_t	numChildren = 0 ;


	// Read two  uint32_t values. One is 12, the other is 10. Then start the properties

	uint32_t pos = 48 +  5 ;

	// Now for the properties!
	
	

	while (pos < totalLen) {

		getDTNode(fd);

	pos =  lseek(fd, 0, SEEK_CUR);
	} // end while
	

	


}