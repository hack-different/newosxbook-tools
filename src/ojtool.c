#include <stdio.h>
#include <string.h>
#include <unistd.h> // access(2)
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>

/* 
 * ojtool (?)
 *
 * What: A simple filter for  otool(1), to resolve %rip relative references
 *       to absolute addresses.
 *
 * Why:  Because IDA is too cumbersome, and Hopper isn't light. Or free.  Sometimes you need a 
 *       simple command line disassembler, and jtool doesn't support i386/x86_64.
 *
 *
 * Usage: otool -tV _x86_64_Mach_O_ | ojtool  -- DEPRECATED
 *     or
 *        ojtool _x86_64_Mach_O_  (Use this for full functionality)
 *
 * How:  This tool reads and buffers lines from otool(1) output. Each line is passed through
 *       verbatim, unless it contains a "(%rip)" string, in which case:
 *
 *         - If the string is prefixed by a symbol, nothing
 *
 *         - If the string is prefixed by a positive or negative hex number, the line is buffered
 *           and only printed once the next line is processed, and its address is obtained, so as
 *           to resolve the simple match of adding the value of RIP.
 *
 *           If the resolved address is found in jtool -d __TEXT.__cstring or __TEXT.__cfstring,
 *           it will also print out the string :-) - for this you need to supply the MachO name.
 *
 *           (otool used to do that, but now doesn't, on some binaries. Go figure. But then, AAPL
 *           rewrote it to itself invoke llvm-* internally. Ugh. Guys, why'd you ruin further what
 *           is/was an already half crummy tool?
 * 
 *        - If the address in the binary is a function (recognized by function_starts) - print it
 *	  - If the address in a call instruction is in stubs, try to resolve stub	
 *
 * Comments: It's a super simple tool. Nothing fancy. I cobbled this together because I find myself
 *           reversing with otool(1) quite a bit, and got tired of perl -e 'printf...'. This could 
 *           probably be nicer in Python. But I loathe Python. 
 * 
 *           YOU NEED JTOOL IN YOUR PATH IF YOU WANT [CF]STRING RESOLUTION
 *
 *           For fat binaries, remember to export ARCH=x86_64
 *  
 *           Now in color :-)
 *
 * Disclaimer: Might be a bug or two here. Works and gets the math right for the binaries I've tested,
 *             x86_64 kernel included. If I ever add x86_64 to jtool (which isn't happening, since only
 *             about five people asked...), this will be obsolete
 *
 * Greets:   Apple. Because otool still disassembles Intel and ARM32 better than I ever could hope to.
 *           But disassembly aside (and ARM64), jtool trumps otool any time.
 *           Who knows - maybe Apple will integrate this super simple logic into otool in 2400 :-)
 * 
 *  	     @JeremyAgost - here's another alternative to those of us who just need a CLI.  (11/05/2016)
 *           Luca - because otool sucks, and jtool still can't do x86_64, but maybe this hybrid
 *                  approach will work in the meanwhile ?
 *
 */

#define RED	"\033[0;31m"
#define PINK    "\e[0;35m"
#define GREEN    "\e[0;32m"
#define CYAN    "\e[0;36m"
#define NORMAL	"\033[0;0m"

int g_Color = 0;

char *ohMyGOT = "";
char *JToolData = "";
char *JToolObjCData = "";
char *JToolFuncStarts = "";
char *JToolComp = "";

#define MAX_LAZY_BINDS	16384
char *lazyBinds[MAX_LAZY_BINDS] = { };
int numLazyBinds = 0;

uint64_t stubsStart, stubsEnd = 0;

void getJToolLazyBindData (char *File)
{
	if (!File) return;
	char cmd[1024];

	// YES THERE IS COMMAND INJECTION HERE. SO WHAT.

	 snprintf (cmd, 1024, "jtool -lazy_bind %s 2>/dev/null", File);
	 FILE *jtoolInput = popen (cmd,"r");
	 // present limit is 4MB of string data...
	 char *data = calloc (1*1024*1024,1); 
	 size_t readBytes = fread(data, // void *restrict ptr, 
				  2*1024*1024, // size_t nitems, 
					1,
				  jtoolInput); // FILE *restrict stream);
	
	pclose(jtoolInput);



	char *newline = NULL;
	
	// skip two first lines
	newline = strchr(data, '\n') ;
	if (!newline) { fprintf(stderr,"Unable to get lazy_bind data from jtool?! No first newline\n");
			return ;}
	newline = strchr(newline +1, '\n') ;
	if (!newline) { fprintf(stderr,"Unable to get lazy_bind data from jtool?! No second newline\n");
			return ;}

	// Ok. Now let's talk.

	int l = 0;
	// Iterate over lazy bind data. Look only for pattern of " _[^ ]\n"
	while ((newline = strchr(newline + 1, '\n')) != NULL) {
		// Go back from the new line to find first space.
		*newline ='\0';
		int back = -1;
		
		while (newline[back] != ' ')
			{
			back--;
			}

		// verify that next is "_"
		if (newline[back+1] != '_') {
			fprintf(stderr,"Warning: Ignoring entry %s\n",
				newline + back + 1);
		}

		lazyBinds[numLazyBinds] = newline+back+1;

		if (getenv ("JDEBUG")) {fprintf(stderr,"Added lazy bind: %s\n",
					lazyBinds[numLazyBinds]); }

		numLazyBinds++;
	}


	

} // getJToolLazyBindData
void getJToolStubData (char *File)
{
	// Ok - this is quick, filthy, but it works:

	// The observation is that the compiler always creates the __TEXT.__stubs
	// (which jtool presently can't read) to contain JMP instructions to the __la_symbol_ptr
	// (which jtool can read, through -lazy_bind)
	
	// So:
	// A) We find the beginning and size of __stubs using strstr on jtool -l.
	//
	// B) We dump all -lazy_bind symbols using jtool into an array
	//
	// C) We calculate the stubs by taking *offset* into __stubs,
	//    then dividing by six (due to jmp), and checking array entry :-)

	if (!File) return;
	 char cmd[1024];

	// YES THERE IS COMMAND INJECTION HERE. SO WHAT.

	 snprintf (cmd, 1024, "jtool -l %s 2>/dev/null", File);
	 FILE *jtoolInput = popen (cmd,"r");
	 // present limit is 4MB of string data...
	 char *data = calloc (1*1024*1024,1); 
	 size_t readBytes = fread(data, // void *restrict ptr, 
				  2*1024*1024, // size_t nitems, 
					1,
				  jtoolInput); // FILE *restrict stream);
	
	pclose(jtoolInput);
	// Look for this pattern: Mem: 0x10004b0fe-0x10004b2e4            __TEXT.__stubs  (Symbol Stubs)

	// if (readBytes <0) { fprintf(stderr,"Error popen(2)ing jtool for stubs\n"); return; }
	 char *stubs = strstr (data, "(Symbol Stubs)");
	 if (!stubs) { fprintf(stderr,"Warning - can't get stubs. Wait till J puts a better mechanism in place..\n");
			return; // not fatal
		     }

	 // seek back "Mem: "
	 int i = (stubs - data);
	 while (i > 0)
	{
		if (memcmp (data +i, "Mem: ",5) == 0)
		{
			// got it! sscanf will work here because that's the way jtool printf()ed it..
			int rc = sscanf(data+i + 5,"0x%llx-0x%llx", &stubsStart, &stubsEnd);
			// but we'll still check

			if (rc !=2) {
				fprintf(stderr,"Warning - Can't figure out stubs from jtool output.. \n");
				return;
				}


			//printf("HERE! stubsStart: %lx, End %lx\n", stubsStart,stubsEnd);

			// Now get lazy bind data
			getJToolLazyBindData(File);

			
			return;
		}
		else i--;

	}

	pclose (jtoolInput);


} 

// @TODO: Just work with companion files!
//

void getJToolFuncStarts(char *File)
{
	// AAPL: Why %$#%$ do LC_FUNCTION_STARTS if otool (now objdump) ignores it?!

	if (!File) return;
	 char cmd[1024];

	 FILE *jtoolInput = popen (cmd,"r");
	 // present limit is 4MB of string data...
	 char *data = calloc (4*1024*1024,1); 
	 size_t readBytes = 0;

	// 05/26 Prefer companion file
	char compFile[10240];
	char *jtooldir = getenv("JTOOLDIR");
	if (jtooldir) { strncpy(compFile,
				jtooldir, 1024);}
	else strcpy(compFile,".");
	strcat (compFile,"/");
	
	// YES THERE IS COMMAND INJECTION HERE. SO WHAT.
	
	 snprintf (cmd, 1024, "jtool -l %s | grep UUID| cut -d':' -f3", File);
	 jtoolInput = popen (cmd,"r");
	 readBytes = fread(data, // void *restrict ptr, 
				  2*1024*1024, // size_t nitems, 
					1,
				  jtoolInput); // FILE *restrict stream);
	 
	 int x = 0;
	 int y = 0;
	 while ((!data[x]) || data[x] == ' ') x++;
	 //data [x] now points to uuid
	 y = x;
	 while (((data[y] != '\n') && data[y] != ' ')) y++;
	 data[y] = '\0';

	 char *bn = basename(File);
	 if (!bn) { perror("basename"); bn = File;}

	 strcat (compFile, bn);
	 strcat (compFile, ".");
	 strcat (compFile, "x86_64.");
 	 strcat (compFile, data+x);
	
	 fprintf(stderr,"Trying companion file : %s\n", compFile);
	 if (access(compFile, R_OK) == 0)
	  {
		
		int fd = open (compFile, O_RDONLY);
		read (fd, data, 2*1024*1024);
		JToolComp = data;
		return;
	 }
	 else
		{
			fprintf(stderr,"Tip: Consider creating a companion file using jtool --jtooldir . -d __DATA.__const > /dev/null\n");
		}



	 pclose(jtoolInput);

	 snprintf (cmd, 1024, "jtool -function_starts %s 2>/dev/null", File);
	 jtoolInput = popen (cmd,"r");
	 // present limit is 4MB of string data...
	 readBytes = fread(data, // void *restrict ptr, 
				  2*1024*1024, // size_t nitems, 
					1,
				  jtoolInput); // FILE *restrict stream);
	 if (readBytes == 2*1024*1024) { 
		fprintf(stderr,"WHOA. More than 2MB of jtool -function_starts ... data. Yikes. Recompile otoolfilt with a larger limit.\n");
		exit(1);
		}

	data[readBytes]='\0'; //important :-)

	JToolFuncStarts = data;

	pclose (jtoolInput);


};

void getJToolData(char *File)
{

	/**
	  * Jtool can't disassemble x86_64; otool can.
	  * otool can't do anything BUT disassemble x86_64, and often ignores string data. Jtool does so much more.
	  *
	  * Combine the two by running jtool -d __... on the binary (for now, only __TEXT.__cstring)
	  * and capture the data.
	  */
	if (!File) return;
	 char cmd[1024];
	// YES THERE IS COMMAND INJECTION HERE. SO WHAT.
	 snprintf (cmd, 1024, "jtool -d __TEXT.__cstring %s 2>/dev/null", File);
	 FILE *jtoolInput = popen (cmd,"r");
	 // present limit is 4MB of string data...
	 char *data = calloc (4*1024*1024,1); 
	 size_t readBytes = fread(data, // void *restrict ptr, 
				  2*1024*1024, // size_t nitems, 
					1,
				  jtoolInput); // FILE *restrict stream);

	 if (readBytes == 2*1024*1024) { 
		fprintf(stderr,"WHOA. More than 2MB of jtool -d ... data. Yikes. Recompile otoolfilt with a larger limit.\n");
		}

	 pclose(jtoolInput);
	 // Append CFString data

	 snprintf (cmd, 1024, "jtool -d __DATA.__cfstring %s 2>/dev/null", File);
	 jtoolInput = popen (cmd,"r");
	 readBytes = fread(data +strlen(data), // void *restrict ptr, 
			 	  1, // size_t size, 
				  2*1024*1024, // size_t nitems, 
				  jtoolInput); // FILE *restrict stream);


	 pclose(jtoolInput);






	// Get the GOT

	 snprintf(cmd, 1024, "JCOLOR=0 jtool -d __DATA.__got %s 2> /dev/null", File);
	 jtoolInput = popen(cmd,"r");
	 jtoolInput = popen (cmd,"r");
	 ohMyGOT = calloc (2,1024*1024);
	 readBytes = fread(ohMyGOT, // void *restrict ptr, 
			 	  1, // size_t size, 
				  2*1024*1024, // size_t nitems, 
				  jtoolInput); // FILE *restrict stream);



	 pclose(jtoolInput);

	 JToolData = data;
}

void getJToolObjCData(char *File)
{

	 char *data = calloc (4*1024*1024,1); 
	if (!File) return;
	 char cmd[1024];
	 // Append CFString data

	 snprintf (cmd, 1024, "jtool -d __DATA.__objc_classrefs %s 2>/dev/null", File);
	 FILE *jtoolInput = popen (cmd,"r");
	 size_t readBytes = fread(data +strlen(data), // void *restrict ptr, 
			 	  1, // size_t size, 
				  2*1024*1024, // size_t nitems, 
				  jtoolInput); // FILE *restrict stream);



	 pclose(jtoolInput);




	 snprintf (cmd, 1024, "jtool -d __DATA.__objc_superrefs %s 2>/dev/null", File);
	 jtoolInput = popen (cmd,"r");
	 readBytes = fread(data +strlen(data), // void *restrict ptr, 
			 	  1, // size_t size, 
				  2*1024*1024, // size_t nitems, 
				  jtoolInput); // FILE *restrict stream);


	 pclose(jtoolInput);




	 snprintf (cmd, 1024, "jtool -d __DATA.__objc_selrefs %s 2>/dev/null", File);
	 jtoolInput = popen (cmd,"r");
	 readBytes = fread(data +strlen(data), // void *restrict ptr, 
			 	  1, // size_t size, 
				  2*1024*1024, // size_t nitems, 
				  jtoolInput); // FILE *restrict stream);




	 pclose(jtoolInput);

	JToolObjCData = data;


}

int main (int argc, char **argv)
{

	g_Color = (getenv ("JCOLOR") != NULL);
	char line[1024];
	int fail = 0;
	int hexNum = 0;

	int rc = 0;
	int otoolKnewThis = 0;
	unsigned long long rip = 0;
	char *saved = NULL;
	char *savedRR = NULL;
	char *ripRef = NULL;

	FILE *input = stdin;


	
	if (argc > 1) {
		// then our next (and only expected) argument is the name of the binary

		// run otool -tV on the binary.
 		// NOTE THIS IS NOT MEANT TO BE SECURE. All I want is to run /usr/bin/otool -tV.
		// Don't you go issuing CVEs on command injection and all that crap.

		char cmd[1024];
		rc = access("/usr/bin/otool", X_OK);

		if (rc < 0) { fprintf(stderr,"I can't find and/or execute otool(1). Is xcode installed?\n");  exit(1);}

		snprintf (cmd, 1024, "/usr/bin/otool -tV %s", argv[1]);
		input = popen (cmd, "r");

		// Get jtool -d __TEXT.__cstring
		getJToolData (argv[1]);
		getJToolObjCData (argv[1]);
		getJToolFuncStarts (argv[1]);
		getJToolStubData (argv[1]);

	}

	fgets (line, 1024, input);
	while (!feof(input))
	{

		rc = sscanf (line, "%llx", &rip);
		
		if (saved)
		{
			// flush any lines from a previous instance, now that we have IP
			fputs (saved, stdout);
			// hexNum + pc

			char resolved[1024];

			sprintf (resolved, "0x%llx:", rip + hexNum);
		
			printf("0x%llx", rip+hexNum);
		

			hexNum = 0;

			
			// But wait - maybe this is a string or something jtool has resolved!
			// The strstr is crude, but efficient - I'm assuming here jtool -d
			// returns data formatted as 0x........: .......

			// Could be GOT
			int otoolKnowsThis =  ( strstr(savedRR,"literal") || strstr(savedRR,"Objc"));

			char *jtoolKnowsThis =NULL;

			char *jtoolKnowsObjC = NULL;
			char *isGOT = NULL;

			if (!otoolKnowsThis)
			{ jtoolKnowsThis = strstr (JToolData,resolved);
			  jtoolKnowsObjC = strstr(JToolObjCData,resolved);
			   isGOT = strstr(ohMyGOT, resolved);
			}
			else {
				isGOT =	jtoolKnowsObjC = jtoolKnowsThis = NULL; 
				}

			if (isGOT) 
			{
				char *newline = strchr (savedRR, '\n');
				if (newline) *newline = '\0';
				fprintf (stdout, "%s",savedRR);
				// Now print what jtool knows...
				newline = strchr (isGOT, '\n');
				*newline = '\0';
				int l = strlen(isGOT) ;
				// GOT is 0000. Curses #@$@#$ a single 0
				// sometimes. so workaround
				while ((l > 0) && (isGOT[l] !='0')) {
					 l--;
				}

				printf("\t ; %s%s%s!\n", 
					g_Color ? GREEN :"",
					isGOT +l +1,
					g_Color ? NORMAL :"");

				*newline = '\n';

				
			}
	

			if (jtoolKnowsObjC)
			{
				// get the \n from savedRR
				char *newline = strchr (savedRR, '\n');
				if (newline) *newline = '\0';
				fprintf (stdout, "%s",savedRR);
				// Now print what jtool knows...
				newline = strchr (jtoolKnowsObjC, '\n');
				*newline = '\0';

				// for sure we have that jtool will display a : somewhere in line

				// Go back from the newline to the first ' ' 

				int l = strlen(jtoolKnowsObjC) -3;
				while (l > 0 && jtoolKnowsObjC[l] !=' ') l--;

				jtoolKnowsObjC+=l;
				printf("\t ; %s%s%s\n", 
					g_Color ? PINK :"",
					jtoolKnowsObjC,
					g_Color ? NORMAL :"");

				*newline = '\n';



			}

			else
			if (jtoolKnowsThis)
			{
				// get the \n from savedRR
				char *newline = strchr (savedRR, '\n');
				if (newline) *newline = '\0';
				fprintf (stdout, "%s",savedRR);
			
				// Now print what jtool knows...
				newline = strchr (jtoolKnowsThis, '\n');
				*newline = '\0';

				// for sure we have that jtool will display a : somewhere in line
				 jtoolKnowsThis = strchr (jtoolKnowsThis , ':') + 2;


				printf("\t ; \"%s%s%s\"\n", 
					g_Color ? RED :"",
					jtoolKnowsThis,
					g_Color ? NORMAL :"");

				*newline = '\n';

		// if (getenv("JCOLOR") ) printf("\e[0;0m");
			}

			else 
			fprintf (stdout, "%s", savedRR);


			free (saved); // no mem leak here :)
			saved = savedRR = NULL;

		}

		ripRef = strstr (line, "(%rip)");

		if (ripRef)
		 	{
			  // get the actual number

			  *ripRef = '\0';
			  char *hexRef = strrchr (line, 'x');

			  if (!hexRef || (hexRef[-1] != '0'))  { 
					   // We can fail if this a symbol(%rip) reference
					   // in which case just restore what was before
					   *ripRef = '(';
					   fail++;
					}
			  else {
			  	hexRef--;
			

			  	rc = sscanf (hexRef, "%x", &hexNum);
			
			 	 if (rc !=1) fail++;

			  	ripRef += 6;
	
				if (hexRef[-1] == '-')
					{

						hexNum = 0 - hexNum;
						hexRef--;
					}
			  
			  	*hexRef = '\0';
				saved = strdup(line);
				savedRR = strdup(ripRef);
				}
			}

		

		
		if (!saved) 
		{
			// 4/4/17: Is this is a known funcstart?
			// want to do this if previous line doesn't end with  ":", meaning otool knew it
			// as an exported symbol
			char temp[16];
			sprintf(temp, ": 0x%llx ", rip);


			char *jtoolKnowsThis = NULL;

		
			if (!otoolKnewThis)
			{
				if (JToolComp)
				{
				sprintf(temp, "0x%llx:", rip);
				jtoolKnowsThis = strstr(JToolComp,temp);
				}
				else
				jtoolKnowsThis = strstr(JToolFuncStarts,temp);

					
			}

			if (jtoolKnowsThis) {
				// Got func  starts - get from end of match to \n..
				char *func = jtoolKnowsThis + strlen(temp);
				char *end = strchr(func, 0xa);
				end[0] = '\0';
				fprintf(stdout, "%s%s%s:\n",
					g_Color ? PINK: "", func,
					g_Color ? NORMAL : "");
				end[0] = 0xa;

			}
		

			// 9/15/17 - much better - look for address rather than instruction

			uint64_t callAddr = 0;
			char *addr = strstr (line, "0x");
			char *otoolKnowsThis =   strstr(line, "##");
			int rc = 0;
			if (addr && !otoolKnowsThis)
			 rc = sscanf (addr, "0x%llx", &callAddr);
			

			// 4/16/17 - is this a callq?

	
			if (callAddr)
			{
				// Try to resolve stub..
				if (rc == 1)
					{
			
	
					// First lookup in cached symbols
					char addrAgain[32] = { 0};
					
					sprintf(addrAgain, "0x%llx:", callAddr);
					jtoolKnowsThis = strstr(JToolComp,addrAgain);

					if (jtoolKnowsThis) {

					char *newline = strchr (jtoolKnowsThis, 0xa);
					jtoolKnowsThis = strstr(jtoolKnowsThis,":") +1;

					//char save = newline[0];
					*addr = '\0';
					
					*newline = '\0';
					char *name = strdup (jtoolKnowsThis);
				         printf ("%s %s%s%s \n",
							line, g_Color? CYAN : "", 
								name,
								g_Color ? NORMAL : "");

					free(name);
					*newline = 0xa;
					}

					else // We have a call, maybe a stub
					if (callAddr >= stubsStart && callAddr <= stubsEnd)
						{
						
				//	 remove the \n
						char *newline = strchr (line,'\n');
						if (newline ) *newline ='\0';

						  int off = ( callAddr - stubsStart)/6;
					
						//@TODO : SANITY: multiple of 6 and not over lazyBinds[l]
						 if ((off < 0) || (off > numLazyBinds)) {
				
							printf("%s ; Error: not a stub (offset %x)\n", line, off);
							
							}
						else {
				
						  	// replace the whole callq part of line!
						  printf ("%s %s%s%s (stub #%d)\n",
							line, g_Color? CYAN : "", 
								lazyBinds[off],
								g_Color ? NORMAL : "",
								 off);
							}

						}

					else { // not a stub, but maybe in funcstarts?

						fputs(line,stdout);

					}
				} // rc == 1
				else { 
	
				// This could be a GOT entry - those have 000000000

	//			sprintf(temp,"0x%llx: 0x0000000000000000", callAddr);
				

					fputs(line,stdout);
				}
			}

			else

			{
			fputs (line, stdout);
		if (strstr (line, ":\n")) { otoolKnewThis = 1; }
		else otoolKnewThis = 0;
			}
		}

		fail = 0;

		fgets (line, 1024, input);

	}

} // end main