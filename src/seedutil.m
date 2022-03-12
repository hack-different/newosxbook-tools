#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

// gcc seedutil.m -o /tmp/sui -framework CoreFoundation -framework Foundation -framework Seeding -F /System/Library/PrivateFrameworks/

@interface SDBuildInfo : NSObject 
	+  (int)currentBuildIsSeed;

@end
@interface SDSeedProgramMigrator :  NSObject

	+ (void)migrateSeedProgramSettings;
	+ (int)fixupSeedProgramSettings;
@end 
@interface SDCatalogUtilities : NSObject 
	+  (void *)_currentCatalog;

@end

@interface SDSeedProgramManager : NSObject 
	+ enrollInSeedProgram:id;
	+ unenrollFromSeedProgram;
	+ (id)currentSeedProgram;
	+ (NSString *)_stringForSeedProgram:id;
	+ _seedProgramForString:NSString;

@end
void usage (void) {
#if 0 
00000001000016bf	pushq	%rbp
00000001000016c0	movq	%rsp, %rbp
#endif
	puts ("usage: seedutil enroll SEED_PROGRAM");

#if 0
00000001000016c3	leaq	0x626(%rip), %rdi ## literal pool for: "usage: seedutil enroll SEED_PROGRAM"
00000001000016ca	callq	0x100001b6e ## symbol stub for: _puts
#endif

	puts ("       seedutil unenroll");
#if 0
00000001000016cf	leaq	0x64a(%rip), %rdi ## literal pool for: "       seedutil unenroll"
00000001000016d6	callq	0x100001b6e ## symbol stub for: _puts
#endif
	puts("       seedutil current");
#if 0
00000001000016db	leaq	0x65e(%rip), %rdi ## literal pool for: "       seedutil current"
00000001000016e2	callq	0x100001b6e ## symbol stub for: _puts
#endif
	puts("       seedutil migrate OLD_VERSION NEW_VERSION");
#if 0
00000001000016e7	leaq	0x672(%rip), %rdi ## literal pool for: "       seedutil migrate OLD_VERSION NEW_VERSION"
00000001000016ee	callq	0x100001b6e ## symbol stub for: _puts
#endif

	puts("       seedutil fixup");
#if 0
00000001000016f3	leaq	0x696(%rip), %rdi ## literal pool for: "       seedutil fixup"
00000001000016fa	popq	%rbp
00000001000016fb	jmp	0x100001b6e ## symbol stub for: _puts
#endif
}


int printProgram() {
#if 0
0000000100001700	pushq	%rbp
0000000100001701	movq	%rsp, %rbp
0000000100001704	pushq	%r15
0000000100001706	pushq	%r14
0000000100001708	pushq	%r13
000000010000170a	pushq	%r12
000000010000170c	pushq	%rbx
000000010000170d	pushq	%rax
#endif

	CFPropertyListRef sp = CFPreferencesCopyValue(CFSTR("SeedProgram"),
			       CFSTR("com.apple.seeding"),
			       kCFPreferencesAnyUser,
			       kCFPreferencesCurrentHost);
#if 0
000000010000170e	movq	0x90b(%rip), %rax ## literal pool symbol address: _kCFPreferencesAnyUser
0000000100001715	movq	(%rax), %r15
0000000100001718	movq	0x909(%rip), %rax ## literal pool symbol address: _kCFPreferencesCurrentHost
000000010000171f	movq	(%rax), %rbx
0000000100001722	leaq	0x95f(%rip), %rdi ## Objc cfstring ref: @"SeedProgram"
0000000100001729	leaq	0x978(%rip), %rsi ## Objc cfstring ref: @"com.apple.seeding"
0000000100001730	movq	%r15, %rdx
0000000100001733	movq	%rbx, %rcx
0000000100001736	callq	0x100001b3e ## symbol stub for: _CFPreferencesCopyValue
000000010000173b	movq	%rax, %r14
#endif

	NSString *sdcat = [SDCatalogUtilities  _currentCatalog];

#if 0
000000010000173e	movq	0xaf3(%rip), %rdi ## Objc class ref: _OBJC_CLASS_$_SDCatalogUtilities
0000000100001745	movq	0xa84(%rip), %rsi ## Objc selector ref: _currentCatalog
000000010000174c	callq	*0x8de(%rip) ## Objc message: +[SDCatalogUtilities _currentCatalog]
0000000100001752	movq	%rax, %rdi
0000000100001755	callq	0x100001b62 ## symbol stub for: _objc_retainAutoreleasedReturnValue
000000010000175a	movq	%rax, %r13
#endif 

	CFPropertyListRef nssfm = CFPreferencesCopyValue(CFSTR("NSShowFeedbackMenu"),
				kCFPreferencesAnyApplication,
			       kCFPreferencesAnyUser,
			       kCFPreferencesCurrentHost);
#if 0
000000010000175d	movq	0x8b4(%rip), %rax ## literal pool symbol address: _kCFPreferencesAnyApplication
0000000100001764	movq	(%rax), %rsi
0000000100001767	leaq	0x95a(%rip), %rdi ## Objc cfstring ref: @"NSShowFeedbackMenu"
000000010000176e	movq	%r15, %rdx
0000000100001771	movq	%rbx, %rcx
0000000100001774	callq	0x100001b3e ## symbol stub for: _CFPreferencesCopyValue
0000000100001779	movq	%rax, %r12
#endif
	CFPropertyListRef dsoo = CFPreferencesCopyValue(CFSTR("DisableSeedOptOut"),
				CFSTR("com.apple.SoftwareUpdate"),
			       kCFPreferencesAnyUser,
			       kCFPreferencesCurrentHost);

#if 0
000000010000177c	leaq	0x965(%rip), %rdi ## Objc cfstring ref: @"DisableSeedOptOut"
0000000100001783	leaq	0x97e(%rip), %rsi ## Objc cfstring ref: @"com.apple.SoftwareUpdate"
000000010000178a	movq	%r15, %rdx
000000010000178d	movq	%rbx, %rcx
0000000100001790	callq	0x100001b3e ## symbol stub for: _CFPreferencesCopyValue
#endif

#if 0 
0000000100001795	movq	%rax, %r15
0000000100001798	movq	0xa39(%rip), %rsi ## Objc selector ref: longValue
000000010000179f	movq	%r14, -0x30(%rbp)
00000001000017a3	movq	%r14, %rdi
00000001000017a6	movq	0x883(%rip), %rbx ## Objc message: -[%rdi longValue]
00000001000017ad	callq	*%rbx
00000001000017af	movq	%rax, %rcx
#endif

	printf("Program: %ld\n", [sp longValue]);
#if 0
00000001000017b2	leaq	0x483(%rip), %rdi ## literal pool for: "Program: %ld\n"
00000001000017b9	xorl	%eax, %eax
00000001000017bb	movq	%rcx, %rsi
00000001000017be	callq	0x100001b68 ## symbol stub for: _printf
#endif

	int bis  =  [SDBuildInfo currentBuildIsSeed];
#if 0
00000001000017c3	movq	0xa76(%rip), %rdi ## Objc class ref: _OBJC_CLASS_$_SDBuildInfo
00000001000017ca	movq	0xa0f(%rip), %rsi ## Objc selector ref: currentBuildIsSeed
00000001000017d1	callq	*%rbx
00000001000017d3	movq	%rbx, %r14
00000001000017d6	testb	%al, %al
#endif
	printf("Build is  seed: %s\n", bis ? "YES" : "NO");
#if 0
00000001000017d8	leaq	0x482(%rip), %rbx ## literal pool for: "NO"
00000001000017df	leaq	0x477(%rip), %rsi ## literal pool for: "YES"
00000001000017e6	cmoveq	%rbx, %rsi
00000001000017ea	leaq	0x459(%rip), %rdi ## literal pool for: "Build is seed: %s\n"
00000001000017f1	xorl	%eax, %eax
00000001000017f3	callq	0x100001b68 ## symbol stub for: _printf
#endif
	printf("CatalogURL: %s\n",  [sdcat UTF8String]);
#if 0
00000001000017f8	movq	%r13, %rdi
00000001000017fb	callq	0x100001b5c ## symbol stub for: _objc_retainAutorelease
0000000100001800	movq	%rax, %r13
0000000100001803	movq	0x9de(%rip), %rsi ## Objc selector ref: UTF8String
000000010000180a	movq	%rax, %rdi
000000010000180d	callq	*%r14
0000000100001810	movq	%rax, %rcx
0000000100001813	leaq	0x44a(%rip), %rdi ## literal pool for: "CatalogURL: %s\n"
000000010000181a	xorl	%eax, %eax
000000010000181c	movq	%rcx, %rsi
000000010000181f	callq	0x100001b68 ## symbol stub for: _printf
#endif

	printf ("NSShowFeedbackMenu: %s\n", nssfm? "YES": "NO");
#if 0
0000000100001824	movq	0x7e5(%rip), %rax ## literal pool symbol address: _kCFBooleanTrue
000000010000182b	movq	(%rax), %r14
000000010000182e	cmpq	%r12, %r14
0000000100001831	movq	%rbx, %rsi
0000000100001834	leaq	0x422(%rip), %rax ## literal pool for: "YES"
000000010000183b	cmoveq	%rax, %rsi
000000010000183f	leaq	0x42e(%rip), %rdi ## literal pool for: "NSShowFeedbackMenu: %s\n"
0000000100001846	xorl	%eax, %eax
0000000100001848	callq	0x100001b68 ## symbol stub for: _printf
#endif

	printf ("DisableSeedOptOut: %s\n", dsoo? "YES": "NO");
#if 0
000000010000184d	cmpq	%r15, %r14
0000000100001850	leaq	0x406(%rip), %rax ## literal pool for: "YES"
0000000100001857	cmoveq	%rax, %rbx
000000010000185b	leaq	0x42a(%rip), %rdi ## literal pool for: "DisableSeedOptOut: %s\n"
0000000100001862	xorl	%eax, %eax
0000000100001864	movq	%rbx, %rsi
0000000100001867	callq	0x100001b68 ## symbol stub for: _printf
000000010000186c	testq	%r12, %r12
000000010000186f	je	0x100001879
0000000100001871	movq	%r12, %rdi
0000000100001874	callq	0x100001b44 ## symbol stub for: _CFRelease
0000000100001879	testq	%r15, %r15
000000010000187c	je	0x100001886
000000010000187e	movq	%r15, %rdi
0000000100001881	callq	0x100001b44 ## symbol stub for: _CFRelease
0000000100001886	movq	0x7ab(%rip), %rbx ## literal pool symbol address: _objc_release
000000010000188d	movq	%r13, %rdi
0000000100001890	callq	*%rbx
0000000100001892	movq	-0x30(%rbp), %rdi
0000000100001896	movq	%rbx, %rax
0000000100001899	addq	$0x8, %rsp
000000010000189d	popq	%rbx
000000010000189e	popq	%r12
00000001000018a0	popq	%r13
00000001000018a2	popq	%r14
00000001000018a4	popq	%r15
00000001000018a6	popq	%rbp
00000001000018a7	jmpq	*%rax
#endif 
 	return 0;
}

int main (int argc, char **argv) {
#if 0
00000001000018a9	pushq	%rbp
00000001000018aa	movq	%rsp, %rbp
00000001000018ad	pushq	%r15
00000001000018af	pushq	%r14
00000001000018b1	pushq	%r13
00000001000018b3	pushq	%r12
00000001000018b5	pushq	%rbx
00000001000018b6	pushq	%rax
00000001000018b7	movq	%rsi, %r13
00000001000018ba	movl	%edi, %ebx
00000001000018bc	callq	0x100001b56 ## symbol stub for: _objc_autoreleasePoolPush
00000001000018c1	movq	%rax, %r15
#endif
	if (getuid()) {
#if 0
00000001000018c4	callq	0x100001b4a ## symbol stub for: _getuid
00000001000018c9	testl	%eax, %eax
00000001000018cb	je	0x1000018e0
#endif
		puts ("Must be run as root");
#if 0
00000001000018cd	leaq	0x57c(%rip), %rdi ## literal pool for: "Must be run as root"
00000001000018d4	callq	0x100001b6e ## symbol stub for: _puts
#endif
		exit(1);
	}

	if (argc == 1) {
#if 0
00000001000018d9	movl	$0x1, %ebx
00000001000018de	jmp	0x1000018ec
#endif
		usage();
		return 0;
	}

#if 0
00000001000018e0	cmpl	$0x1, %ebx
00000001000018e3	jg	0x100001905
00000001000018e5	callq	0x1000016bf
00000001000018ea	xorl	%ebx, %ebx
00000001000018ec	movq	%r15, %rdi
00000001000018ef	callq	0x100001b50 ## symbol stub for: _objc_autoreleasePoolPop
00000001000018f4	movl	%ebx, %eax
00000001000018f6	addq	$0x8, %rsp
00000001000018fa	popq	%rbx
00000001000018fb	popq	%r12
00000001000018fd	popq	%r13
00000001000018ff	popq	%r14
0000000100001901	popq	%r15
0000000100001903	popq	%rbp
0000000100001904	retq
#endif

	NSString *arg = [NSString stringWithUTF8String:argv[1] ];

#if 0
0000000100001905	movq	0x93c(%rip), %rdi ## Objc class ref: _OBJC_CLASS_$_NSString
000000010000190c	movq	0x8(%r13), %rdx
0000000100001910	movq	0x8d9(%rip), %rsi ## Objc selector ref: stringWithUTF8String:
0000000100001917	movq	0x712(%rip), %r14 ## Objc message: +[NSString stringWithUTF8String:]
000000010000191e	callq	*%r14
#endif 

#if 0
0000000100001921	movq	%rax, %rdi
0000000100001924	callq	0x100001b62 ## symbol stub for: _objc_retainAutoreleasedReturnValue
#endif

	if ([arg isEqualToString:@"enroll"]) 
{
#if 0
0000000100001929	movq	%rax, %r12
000000010000192c	movq	0x8c5(%rip), %rsi ## Objc selector ref: isEqualToString:
0000000100001933	leaq	0x7ee(%rip), %rdx ## Objc cfstring ref: @"enroll"
000000010000193a	movq	%rax, %rdi
000000010000193d	callq	*%r14
0000000100001940	testb	%al, %al
0000000100001942	je	0x100001953
#endif
		if (argc > 2) {
		printf("Enrolling...\n");
#if 0
000000010000199d	leaq	0x49d(%rip), %rdi ## literal pool for: "Enrolling...\n"
00000001000019a4	callq	0x100001b6e ## symbol stub for: _puts
#endif
	[SDSeedProgramManager
		enrollInSeedProgram: [SDSeedProgramManager _seedProgramForString:[NSString stringWithUTF8String:argv[1]]]];

#if 0
00000001000019a9	movq	0x8a0(%rip), %r14 ## Objc class ref: _OBJC_CLASS_$_SDSeedProgramManager
00000001000019b0	movq	0x891(%rip), %rdi ## Objc class ref: _OBJC_CLASS_$_NSString
00000001000019b7	movq	0x10(%r13), %rdx
00000001000019bb	movq	0x82e(%rip), %rsi ## Objc selector ref: stringWithUTF8String:
00000001000019c2	movq	0x667(%rip), %rax ## Objc message: +[NSString stringWithUTF8String:]
00000001000019c9	movq	%rax, %r13
00000001000019cc	callq	*%rax
00000001000019ce	movq	%rax, %rdi
00000001000019d1	callq	0x100001b62 ## symbol stub for: _objc_retainAutoreleasedReturnValue
00000001000019d6	movq	%rax, %rbx
00000001000019d9	movq	0x820(%rip), %rsi ## Objc selector ref: _seedProgramForString:
00000001000019e0	movq	%r14, %rdi
00000001000019e3	movq	%rax, %rdx
00000001000019e6	callq	*%r13
00000001000019e9	movq	%rax, %r14
00000001000019ec	movq	%rbx, %rdi
00000001000019ef	callq	*0x643(%rip) ## literal pool symbol address: _objc_release
00000001000019f5	movq	0x854(%rip), %rdi ## Objc class ref: _OBJC_CLASS_$_SDSeedProgramManager
00000001000019fc	movq	0x805(%rip), %rsi ## Objc selector ref: enrollInSeedProgram:
0000000100001a03	movq	%r14, %rdx
0000000100001a06	callq	*%r13
0000000100001a09	callq	0x100001700
0000000100001a0e	movq	%r12, %rbx
0000000100001a11	jmp	0x100001ad6
#endif
		}

		else {  usage(); exit(1);}
#if 0
0000000100001944	cmpl	$0x2, %ebx
0000000100001947	jg	0x10000199d
0000000100001949	callq	0x1000016bf
000000010000194e	jmp	0x100001a0e
#endif
}
	if ([arg isEqualToString:@"unenroll"]) {
#if 0
0000000100001953	leaq	0x7ee(%rip), %rdx ## Objc cfstring ref: @"unenroll"
000000010000195a	movq	%r12, %rdi
000000010000195d	movq	0x894(%rip), %r14 ## Objc selector ref: isEqualToString:
0000000100001964	movq	%r14, %rsi
0000000100001967	callq	*0x6c3(%rip) ## Objc message: -[%rdi isEqualToString:]
000000010000196d	testb	%al, %al
000000010000196f	movq	%r12, %rbx
0000000100001972	je	0x100001a16
#endif
	puts ("Unenrolling...\n");
#if 0
0000000100001978	leaq	0x4b2(%rip), %rdi ## literal pool for: "Unenrolling...\n"
000000010000197f	callq	0x100001b6e ## symbol stub for: _puts
#endif
	[SDSeedProgramManager unenrollFromSeedProgram];
#if 0
0000000100001984	movq	0x8c5(%rip), %rdi ## Objc class ref: _OBJC_CLASS_$_SDSeedProgramManager
000000010000198b	movq	0x87e(%rip), %rsi ## Objc selector ref: unenrollFromSeedProgram
0000000100001992	callq	*0x698(%rip) ## Objc message: +[SDSeedProgramManager unenrollFromSeedProgram]
0000000100001998	jmp	0x100001ad1
#endif 
}

	else if ([arg isEqualToString:@"current"]) 
#if 0
0000000100001a16	leaq	0x74b(%rip), %rdx ## Objc cfstring ref: @"current"
0000000100001a1d	movq	%rbx, %rdi
0000000100001a20	movq	%r14, %rsi
0000000100001a23	callq	*0x607(%rip) ## Objc message: +[SDSeedProgramManager enrollInSeedProgram:]
0000000100001a29	testb	%al, %al
0000000100001a2b	je	0x100001a9a
#endif
	{
		void *csp = [SDSeedProgramManager currentSeedProgram];

#if 0
0000000100001a2d	movq	0x81c(%rip), %rdi ## Objc class ref: _OBJC_CLASS_$_SDSeedProgramManager
0000000100001a34	movq	0x7dd(%rip), %rsi ## Objc selector ref: currentSeedProgram
0000000100001a3b	movq	0x5ee(%rip), %rax ## Objc message: +[SDSeedProgramManager currentSeedProgram]
0000000100001a42	movq	%rax, %r12
0000000100001a45	callq	*%rax
#endif 

		NSString *str = [SDSeedProgramManager _stringForSeedProgram:csp];

#if 0
0000000100001a47	movq	0x802(%rip), %rdi ## Objc class ref: _OBJC_CLASS_$_SDSeedProgramManager
0000000100001a4e	movq	0x7cb(%rip), %rsi ## Objc selector ref: _stringForSeedProgram:
0000000100001a55	movq	%rax, %rdx
0000000100001a58	callq	*%r12
0000000100001a5b	movq	%rax, %rdi
0000000100001a5e	callq	0x100001b62 ## symbol stub for: _objc_retainAutoreleasedReturnValue
0000000100001a63	movq	%rax, %rdi
0000000100001a66	callq	0x100001b5c ## symbol stub for: _objc_retainAutorelease
#endif

		printf("Currently enrolled in: %s\n\n",
			[str  UTF8String]);
#if 0 
0000000100001a6b	movq	%rax, %r14
0000000100001a6e	movq	0x773(%rip), %rsi ## Objc selector ref: UTF8String
0000000100001a75	movq	%rax, %rdi
0000000100001a78	callq	*%r12
0000000100001a7b	movq	%rax, %rcx
0000000100001a7e	leaq	0x236(%rip), %rdi ## literal pool for: "Currently enrolled in: %s\n\n"
0000000100001a85	xorl	%eax, %eax
0000000100001a87	movq	%rcx, %rsi
0000000100001a8a	callq	0x100001b68 ## symbol stub for: _printf
0000000100001a8f	movq	%r14, %rdi
0000000100001a92	callq	*0x5a0(%rip) ## literal pool symbol address: _objc_release
0000000100001a98	jmp	0x100001ad1
#endif
	printProgram ();
	}
	// @migrate
	if ([arg isEqualToString:@"migrate"]) 
	{
#if 0
0000000100001a9a	leaq	0x6e7(%rip), %rdx ## Objc cfstring ref: @"migrate"
0000000100001aa1	movq	%rbx, %rdi
0000000100001aa4	movq	%r14, %rsi
0000000100001aa7	callq	*0x583(%rip) ## Objc message: +[SDSeedProgramManager UTF8String]
0000000100001aad	testb	%al, %al
0000000100001aaf	je	0x100001aee
#endif
		printf("Migrating seed program settings\n\n");
#if 0
0000000100001ab1	leaq	0x358(%rip), %rdi ## literal pool for: "Migrating seed program settings\n"
0000000100001ab8	callq	0x100001b6e ## symbol stub for: _puts
#endif
		[SDSeedProgramMigrator migrateSeedProgramSettings];

#if 0
0000000100001abd	movq	0x794(%rip), %rdi ## Objc class ref: _OBJC_CLASS_$_SDSeedProgramMigrator
0000000100001ac4	movq	0x75d(%rip), %rsi ## Objc selector ref: migrateSeedProgramSettings
0000000100001acb	callq	*0x55f(%rip) ## Objc message: +[SDSeedProgramMigrator migrateSeedProgramSettings]

#endif
	printProgram ();
	
	}

#if 0
0000000100001ad1	callq	0x100001700
0000000100001ad6	movq	%rbx, %rdi
0000000100001ad9	callq	*0x559(%rip) ## literal pool symbol address: _objc_release
0000000100001adf	movq	%r15, %rdi
0000000100001ae2	callq	0x100001b50 ## symbol stub for: _objc_autoreleasePoolPop
0000000100001ae7	xorl	%ebx, %ebx
0000000100001ae9	jmp	0x1000018f4
#endif
	else if ([arg isEqualToString:@"fixup"]) 
#if 0
0000000100001aee	leaq	0x6b3(%rip), %rdx ## Objc cfstring ref: @"fixup"
0000000100001af5	movq	%rbx, %rdi
0000000100001af8	movq	%r14, %rsi
0000000100001afb	callq	*0x52f(%rip) ## Objc message: -[%rdi migrateSeedProgramSettings]
0000000100001b01	testb	%al, %al
0000000100001b03	je	0x100001b37
#endif
	{

	printf("Fixing up seed program settings\n");
#if 0
0000000100001b05	leaq	0x2a4(%rip), %rdi ## literal pool for: "Fixing up seed program settings\n"
0000000100001b0c	callq	0x100001b6e ## symbol stub for: _puts
#endif
	if (![SDSeedProgramMigrator fixupSeedProgramSettings])
#if 0
0000000100001b11	movq	0x740(%rip), %rdi ## Objc class ref: _OBJC_CLASS_$_SDSeedProgramMigrator
0000000100001b18	movq	0x711(%rip), %rsi ## Objc selector ref: fixupSeedProgramSettings
0000000100001b1f	callq	*0x50b(%rip) ## Objc message: +[SDSeedProgramMigrator fixupSeedProgramSettings]
0000000100001b25	testb	%al, %al
0000000100001b27	jne	0x100001ad1
#endif
		printf("No seed program was set; did nothing\n");
#if 0
0000000100001b29	leaq	0x2b0(%rip), %rdi ## literal pool for: "No seed program was set; did nothing"
0000000100001b30	callq	0x100001b6e ## symbol stub for: _puts
0000000100001b35	jmp	0x100001ad6
0000000100001b37	callq	0x1000016bf
0000000100001b3c	jmp	0x100001ad6
#endif
	}
	return 0;
}