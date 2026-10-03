/* Opaque in-process plugin transport. Amiberry owns guest-memory copies,
 * byte limits and reset hooks, not command parsing or GL resources. */
#ifndef AMIBERRY_SERVICE_TRANSPORT_H
#define AMIBERRY_SERVICE_TRANSPORT_H

#include "uae/types.h"

struct TrapContext;

// Audited generic service transport trap selector
constexpr uae_u32 SERVICE_TRANSPORT_TRAP = 120;

// Transport status / error codes
constexpr uae_u32 SERVICE_TRANSPORT_ERROR_UNAVAILABLE = 0xffffffffu;
constexpr uae_u32 SERVICE_TRANSPORT_ERROR_INVALID     = 0xfffffffeu;
constexpr uae_u32 SERVICE_TRANSPORT_ERROR_CAPACITY    = 0xfffffffdu;
constexpr uae_u32 SERVICE_TRANSPORT_ERROR_LOST        = 0xfffffffcu; // context lost, or a reset overtook the call
constexpr uae_u32 SERVICE_TRANSPORT_MAX_TRANSFER_BYTES = 64u * 1024u * 1024u;
// Operations: 0=query, 1=create, 2=submit, 3=readback, 4=destroy, 5=request,
// 6=request with an in-place reply (fails with UNAVAILABLE unless the reply is
// directly addressable; written RTG memory is marked dirty for the display).
// Requests: arg2=handle, arg3=request pointer, arg4=request length,
// arg5=guest pointer to {reply pointer, reply length}, two big-endian u32s.

// Generic trap entry point
uae_u32 service_transport_uaelib(TrapContext* ctx, uae_u32 operation, uae_u32 arg2,
    uae_u32 arg3, uae_u32 arg4, uae_u32 arg5);

// Result when native-code access is disabled
uae_u32 service_transport_native_code_denied_result(uae_u32 operation);

// Invalidation / reset hooks
void service_transport_reset();

#endif
