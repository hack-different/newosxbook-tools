typedef struct sbParams sbParams_t;

extern sbParams_t *sandbox_create_params(void);
int    sandbox_set_param (sbParams_t    *params, char *param, char *value);


typedef struct sbProfile {
        char    *name;
        void    *blob;
        int32_t  len;

} sbProfile_t;


extern sbProfile_t *sandbox_compile_file(char *filename, sbParams_t *params, char **err);
extern sbProfile_t *sandbox_compile_string(char *profile_string, sbParams_t *params, char **err);
extern sbProfile_t *sandbox_compile_entitlements(char *ents, sbParams_t *params, char **err);
extern sbProfile_t *sandbox_compile_named(char *profile_name, sbParams_t *params, char **err);
#ifdef SB459
extern int sandbox_set_trace_path (sbProfile_t *, char *Path) __attribute__((weak_import));;
extern int sandbox_vtrace_enable(void);
extern char *sandbox_vtrace_report(void);
#endif
extern void sandbox_free_profile(sbProfile_t *);
extern int sandbox_apply_container(sbProfile_t *, uint32_t);



char *operation_names[] = {
	"default",
	"appleevent-send",
	"authorization-right-obtain",
	"device*",
	"device-camera",
	"device-microphone",
	"distributed-notification-post",
	"file*",
	"file-chroot",
	"file-ioctl",
	"file-issue-extension",
	"file-map-executable",
	"file-mknod",
	"file-mount",
	"file-read*",
	"file-read-data",
	"file-read-metadata",
	"file-read-xattr",
	"file-revoke",
	"file-search",
	"file-unmount",
	"file-write*",
	"file-write-create",
	"file-write-data",
	"file-write-flags",
	"file-write-mode",
	"file-write-owner",
	"file-write-setugid",
	"file-write-times",
	"file-write-unlink",
	"file-write-xattr",
	"generic-issue-extension",
	"qtn-user",
	"qtn-download",
	"qtn-sandbox",
	"hid-control",
	"iokit*",
	"iokit-issue-extension",
	"iokit-open",
	"iokit-set-properties",
	"iokit-get-properties",
	"ipc*",
	"ipc-posix*",
	"ipc-posix-issue-extension",
	"ipc-posix-sem",
	"ipc-posix-shm*",
	"ipc-posix-shm-read*",
	"ipc-posix-shm-read-data",
	"ipc-posix-shm-read-metadata",
	"ipc-posix-shm-write*",
	"ipc-posix-shm-write-create",
	"ipc-posix-shm-write-data",
	"ipc-posix-shm-write-unlink",
	"ipc-sysv*",
	"ipc-sysv-msg",
	"ipc-sysv-sem",
	"ipc-sysv-shm",
	"job-creation",
	"load-unsigned-code",
	"lsopen",
	"mach*",
	"mach-bootstrap",
	"mach-issue-extension",
	"mach-lookup",
	"mach-per-user-lookup",
	"mach-priv*",
	"mach-priv-host-port",
	"mach-priv-task-port",
	"mach-register",
	"mach-task-name",
	"network*",
	"network-inbound",
	"network-bind",
	"network-outbound",
	"user-preference*",
	"user-preference-read",
	"user-preference-write",
	"process*",
	"process-exec*",
	"process-exec-interpreter",
	"process-fork",
	"process-info*",
	"process-info-listpids",
	"process-info-pidinfo",
	"process-info-pidfdinfo",
	"process-info-pidfileportinfo",
	"process-info-setcontrol",
	"process-info-dirtycontrol",
	"process-info-rusage",
	"pseudo-tty",
	"signal",
	"sysctl*",
	"sysctl-read",
	"sysctl-write",
	"system*",
	"system-acct",
	"system-audit",
	"system-chud",
	"system-debug",
	"system-fsctl",
	"system-info",
	"system-kext*",
	"system-kext-load",
	"system-kext-unload",
	"system-lcid",
	"system-mac-label",
	"system-nfssvc",
	"system-privilege",
	"system-reboot",
	"system-sched",
	"system-set-time",
	"system-socket",
	"system-suspend-resume",
	"system-swap",
	"system-write-bootstrap",
	NULL}; // important :-)

// The vararg definition is important because args x3 and above go on the stack
extern int sandbox_check (int Pid, char *Op, int flags, ...);
extern int sandbox_container_path_for_pid (int Pid, char *Buf, int Len);
extern int sandbox_suspend(int Pid);
extern int sandbox_unsuspend(int Pid);

// Constants figured out by reversing authd and comparing with open source of Security.framework
#define SANDBOX_FILTER_PATH     0x1
#define SANDBOX_FILTER_RIGHT_NAME 0x2

extern int SANDBOX_CHECK_NO_REPORT;

extern int __sandbox_ms(char *Label, int Op, void *ptr,...);

