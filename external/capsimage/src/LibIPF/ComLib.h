#undef DllSub
#undef DllVar
#undef ExtSub
#undef ExtVar

#ifdef LIB_USER

#define DllSub DllImport
#define DllVar DllImport

#else

#define DllSub DllExport
#define DllVar extern DllExport

#endif

#define ExtSub extern "C" DllSub
#define ExtVar extern "C" DllVar

#ifndef _WIN32
#undef ExtSub
#define ExtSub
#ifndef __cdecl
#define __cdecl __attribute__((cdecl))
#endif
#endif
