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

