

/* Functions return codes */
#define NO_PROBLEM 0
#define DMS_FILE_END 1
#define ERR_NOMEMORY 2
#define ERR_CANTOPENIN 3
#define ERR_CANTOPENOUT 4
#define ERR_NOTDMS 5
#define ERR_SREAD 6
#define ERR_HCRC 7
#define ERR_NOTTRACK 8
#define ERR_BIGTRACK 9
#define ERR_THCRC 10
#define ERR_TDCRC 11
#define ERR_CSUM 12
#define ERR_CANTWRITE 13
#define ERR_BADDECR 14
#define ERR_UNKNMODE 15
#define ERR_NOPASSWD 16
#define ERR_BADPASSWD 17
#define ERR_FMS 18
#define ERR_GZIP 19
#define ERR_READDISK 20


/* Command to execute */
#define CMD_VIEW 1
#define CMD_VIEWFULL 2
#define CMD_SHOWDIZ 3
#define CMD_SHOWBANNER 4
#define CMD_TEST 5
#define CMD_UNPACK 6
#define CMD_UNPKGZ 7
#define CMD_EXTRACT 8


#define OPT_VERBOSE 1
#define OPT_QUIET 2

#define DMS_EXTRA_SIZE 10

USHORT DMS_Process_File(struct zfile *, struct zfile *, USHORT, USHORT, USHORT, USHORT, int, struct zfile **extra);
