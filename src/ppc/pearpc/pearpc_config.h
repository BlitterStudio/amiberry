
#ifdef WIN64
#define SYSTEM_ARCH_SPECIFIC_ENDIAN_DIR "system/arch/x86_64/sysendian.h"
#else
#define SYSTEM_ARCH_SPECIFIC_ENDIAN_DIR "system/arch/x86/sysendian.h"
#endif

#define HOST_ENDIANESS HOST_ENDIANESS_LE
