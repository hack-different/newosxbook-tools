#include <mach/mach.h>
#include <dispatch/dispatch.h>

// These would go in ktrace.h, if Apple ever provided it:
typedef struct trace_point {  
    uint64_t        timestamp;
    uintptr_t       arg1, arg2, arg3, arg4, arg5;
    uint32_t        debugid;
#if defined(__LP64__)
    uint32_t        cpuid;
    uintptr_t       unused;
#endif
} *ktrace_event_t;

typedef void           *ktrace_session_t;
ktrace_session_t        ktrace_session_create(void);
char *ktrace_name_for_eventid(ktrace_session_t s, uint32_t eventid);
void ktrace_events_all(ktrace_session_t S, void (^)(ktrace_event_t tp));

int main (int argc,char **argv) {
	// Example: Simple kdebug client. Note this is intentionally
	// unfiltered - you can use ktrace_events_class, .._single, .._range
	// and other more specific calls in a similar manner.
 	ktrace_session_t   s=     ktrace_session_create();

 	ktrace_events_all(s,  ^(ktrace_event_t tp) {
		printf("IN TRACE Code %x, Arg1: %llx Name %s\n",
			 tp->debugid, tp->arg1,
			ktrace_name_for_eventid(s,tp->debugid));
    	});

	int rc = ktrace_start(s, dispatch_get_main_queue());
      // A more complete sample would use ktrace_set_signal_handler to
      // install a handler and call ktrace_end, to ensure resources are freed
	dispatch_main();
} 