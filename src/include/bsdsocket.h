 /*
  * UAE - The Un*x Amiga Emulator
  *
  * bsdsocket.library emulation
  *
  * Copyright 1997,98 Mathias Ortmann
  *
  */

#ifndef UAE_BSDSOCKET_H
#define UAE_BSDSOCKET_H

#include "uae/types.h"

#define BSD_TRACING_ENABLED 0

#define ISBSDTRACE (BSD_TRACING_ENABLED) 
#define BSDTRACE(x) do { if (ISBSDTRACE) { write_log x; } } while(0)

extern int init_socket_layer (void);
extern void deinit_socket_layer (void);

/* inital size of per-process descriptor table (currently fixed) */
#define DEFAULT_DTABLE_SIZE 64

#define SCRATCHBUFSIZE 128

#define MAXPENDINGASYNC 512

#define MAXADDRLEN 256

#ifdef _WIN32
#define SOCKET_TYPE SOCKET
#else
#define SOCKET_TYPE int
#endif

/* allocated and maintained on a per-task basis */
struct socketbase {
    struct socketbase *next;
    struct socketbase *nextsig;	/* queue for tasks to signal */

    uaecptr sysbase;
	int dosignal;		/* signal flag */
    uae_u32 ownertask;		/* task that opened the library */
    int signal;			/* signal allocated for that task */
    int sb_errno, sb_herrno;	/* errno and herrno variables */
    uae_u32 errnoptr, herrnoptr;	/* pointers */
    uae_u32 errnosize, herrnosize;	/* pinter sizes */
    int dtablesize;		/* current descriptor/flag etc. table size */
    SOCKET_TYPE *dtable;	/* socket descriptor table */
    int *ftable;		/* socket flags */
    int resultval;
    uae_u32 hostent;		/* pointer to the current hostent structure (Amiga mem) */
    uae_u32 hostentsize;
    uae_u32 protoent;		/* pointer to the current protoent structure (Amiga mem) */
    uae_u32 protoentsize;
    uae_u32 servent;		/* pointer to the current servent structure (Amiga mem) */
    uae_u32 serventsize;
    uae_u32 sigstosend;
    uae_u32 eventsigs;		/* EVENT sigmask */
    uae_u32 eintrsigs;		/* EINTR sigmask */
    int eintr;			/* interrupted by eintrsigs? */
    int eventindex;		/* current socket looked at by GetSocketEvents() to prevent starvation */
	uae_u32 logstat;
	uae_u32 logptr;
	uae_u32 logmask;
	uae_u32 logfacility;
	uaecptr fdcallback;

    unsigned int *mtable;	/* window messages allocated for asynchronous event notification */
    /* host-specific fields below */
#ifdef _WIN32
    SOCKET_TYPE sockAbort;	/* for aborting WinSock2 select() (damn Microsoft) */
    SOCKET_TYPE sockAsync;	/* for aborting WSBAsyncSelect() in window message handler */
    int needAbort;		/* abort flag */
    void *hAsyncTask;		/* async task handle */
    void *hEvent;		/* thread event handle */
#else
    uae_sem_t sem;		/* semaphore to notify the socket thread of work */
    uae_thread_id thread;	/* socket thread */
    int  sockabort[2];		/* pipe used to tell the thread to abort a select */
    int action;
    int s;			/* for accept */
    uae_u32 name;		/* For gethostbyname */
    uae_u32 a_addr;		/* gethostbyaddr, accept */
    uae_u32 a_addrlen;		/* for gethostbyaddr, accept */
    uae_u32 flags;
    void *buf;
    uae_u32 len;
    uae_u32 to, tolen, from, fromlen;
    int nfds;
    uae_u32 sets [3];
    uae_u32 timeout;
    uae_u32 sigmp;
#endif
    TrapContext *context;
};

#define LIBRARY_SIZEOF 36

struct UAEBSDBase {
    uae_u8 dummy[LIBRARY_SIZEOF];
    struct socketbase *sb;
    uae_u8 scratchbuf[SCRATCHBUFSIZE];
};

/* socket flags */
/* socket events to report */
#define REP_ACCEPT	 0x01	/* there is a connection to accept() */
#define REP_CONNECT	 0x02	/* connect() completed */
#define REP_OOB		 0x04	/* socket has out-of-band data */
#define REP_READ	 0x08	/* socket is readable */
#define REP_WRITE	 0x10	/* socket is writeable */
#define REP_ERROR	 0x20	/* asynchronous error on socket */
#define REP_CLOSE	 0x40	/* connection closed (graceful or not) */
#define REP_ALL      0x7f
/* socket events that occurred */
#define SET_ACCEPT	 0x0100	/* there is a connection to accept() */
#define SET_CONNECT	 0x0200	/* connect() completed */
#define SET_OOB		 0x0400	/* socket has out-of-band data */
#define SET_READ	 0x0800	/* socket is readable */
#define SET_WRITE	 0x1000	/* socket is writeable */
#define SET_ERROR	 0x2000	/* asynchronous error on socket */
#define SET_CLOSE	 0x4000	/* connection closed (graceful or not) */
#define SET_ALL      0x7f00
/* socket properties */
#define SF_BLOCKING 0x80000000
#define SF_BLOCKINGINPROGRESS 0x40000000
#define SF_DGRAM 0x20000000
/* STBC_FDCALLBACK */
#define FDCB_FREE  0
#define FDCB_ALLOC 1
#define FDCB_CHECK 2

uae_u32 addstr(TrapContext *ctx, uae_u32 * dst, const TCHAR *src);
uae_u32 addstr_ansi(TrapContext *ctx, uae_u32 * dst, const uae_char *src);
uae_u32 strncpyha(TrapContext *ctx, uae_u32 dst, const uae_char *src, int size);
uae_u32 addmem(TrapContext *ctx, uae_u32 * dst, const uae_char *src, int len);

#define SB struct socketbase *sb

extern void bsdsocklib_seterrno(TrapContext*, SB, int);
extern void bsdsocklib_setherrno(TrapContext*, SB, int);

extern void sockabort (SB);

extern void addtosigqueue (SB, int);
extern void removefromsigqueue (SB);
extern void sigsockettasks (void);
extern void locksigqueue (void);
extern void unlocksigqueue (void);

extern bool checksd(TrapContext*, SB, int sd);
extern void setsd(TrapContext*, SB, int , SOCKET_TYPE);
extern int getsd (TrapContext*, SB, SOCKET_TYPE);
extern SOCKET_TYPE getsock (TrapContext*, SB, int);
extern void releasesock (TrapContext*, SB, int);

extern void waitsig (TrapContext *context, SB);
extern void cancelsig (TrapContext *context, SB);

extern int host_sbinit (TrapContext*, SB);
extern void host_sbcleanup (SB);
extern void host_sbreset (void);
extern void host_closesocketquick (SOCKET_TYPE);

extern int host_dup2socket (TrapContext *, SB, int, int);
extern int host_socket (TrapContext *, SB, int, int, int);
extern uae_u32 host_bind (TrapContext *, SB, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_listen (TrapContext *, SB, uae_u32, uae_u32);
extern void host_accept (TrapContext *, SB, uae_u32, uae_u32, uae_u32);
extern void host_sendto (TrapContext *, SB, uae_u32, uae_u32, uae_u8*, uae_u32, uae_u32, uae_u32, uae_u32);
extern void host_recvfrom (TrapContext *, SB, uae_u32, uae_u32, uae_u8*, uae_u32, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_shutdown (TrapContext *,SB, uae_u32, uae_u32);
extern void host_setsockopt (TrapContext *, SB, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_getsockopt (TrapContext *, SB, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_getsockname (TrapContext *, SB, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_getpeername (TrapContext *, SB, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_IoctlSocket (TrapContext *, SB, uae_u32, uae_u32, uae_u32);
extern int host_CloseSocket (TrapContext *, SB, int);
extern void host_connect (TrapContext *, SB, uae_u32, uae_u32, uae_u32);
extern void host_WaitSelect (TrapContext *, SB, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32);
extern uae_u32 host_SetSocketSignals (void);
extern uae_u32 host_getdtablesize (void);
extern uae_u32 host_ObtainSocket (void);
extern uae_u32 host_ReleaseSocket (void);
extern uae_u32 host_ReleaseCopyOfSocket (void);
extern uae_u32 host_Inet_NtoA(TrapContext *ctx, SB, uae_u32);
extern uae_u32 host_inet_addr(TrapContext *ctx, uae_u32);
extern uae_u32 host_Inet_LnaOf (void);
extern uae_u32 host_Inet_NetOf (void);
extern uae_u32 host_Inet_MakeAddr (void);
extern uae_u32 host_inet_network (void);
extern void host_gethostbynameaddr (TrapContext *, SB, uae_u32, uae_u32, long);
extern uae_u32 host_getnetbyname (void);
extern uae_u32 host_getnetbyaddr (void);
extern void host_getservbynameport (TrapContext *, SB, uae_u32, uae_u32, uae_u32);
extern void host_getprotobyname (TrapContext *, SB, uae_u32);
extern void host_getprotobynumber (TrapContext *, SB, uae_u32);
extern uae_u32 host_vsyslog (void);
extern uae_u32 host_Dup2Socket (void);
extern uae_u32 host_gethostname(TrapContext *ctx, uae_u32, uae_u32);
extern uae_u32 callfdcallback (TrapContext *context, SB, uae_u32 fd, uae_u32 action);

extern uaecptr bsdlib_startup (TrapContext*, uaecptr);
extern void bsdlib_install (void);
extern void bsdlib_reset (void);

void bsdsock_fake_int_handler(void);

extern int volatile bsd_int_requested;

#endif /* UAE_BSDSOCKET_H */
