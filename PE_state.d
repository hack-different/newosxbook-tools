#!/usr/sbin/dtrace -s
#pragma D option quiet

BEGIN
{
    self->boot_args = ((struct boot_args*)(`PE_state).bootArgs);
    self->deviceTreeHead = ((struct boot_args*)(`PE_state).deviceTreeHead);
    self->video = ((PE_Video ) (`PE_state).video);

    printf("EFI: %d-bit\n", self->boot_args->efiMode);
    printf("Video: Base Addr: %p\n", self->video.v_baseAddr);
    printf("Video is in %s mode\n", (self->video.v_display == 1 ? "Graphics" : "Text"));
	printf ("Runtime services is %p\n", (self->boot_args->efiRuntimeServicesPageStart));
    printf("Video resolution: %dx%dx%d\n", self->video.v_width, self->video.v_height, self->video.v_depth);

    printf ("Kernel command line : %s\n", self->boot_args->CommandLine);

    printf ("Kernel begins at physical address 0x%x and spans %d bytes\n", 
	   self->boot_args->kaddr, self->boot_args->ksize); 
    printf ("Device tree begins at physical address 0x%x and spans %d bytes\n", 
	   self->boot_args->deviceTreeP, self->boot_args->deviceTreeLength); 
 
    printf ("Memory Map of %d bytes resides in physical address 0x%x\n",
	    self->boot_args->MemoryMapSize,
	    self->boot_args->MemoryMap);

    printf ("Kernel Slide: %x\n", self->boot_args->kslide);

    printf("Physical memory size: %d\n",self->boot_args->PhysicalMemorySize);

    printf("FSB Frequency: %d\n",self->boot_args->FSBFrequency);
    printf("CSR (SIP) capabilities: 0x%x\t ActiveConfig: 0x%x\n", self->boot_args->csrCapabilities,self->boot_args->csrActiveConfig);
    printf("APFS Data: 0x%x-0x%x\n", self->boot_args->apfsDataStart, self->boot_args->apfsDataStart + self->boot_args->apfsDataSize);
    exit(0);
}

