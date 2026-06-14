/*
 * SPDX-FileCopyrightText: 2026 Dimitris Panokostas
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UAE_MHI_HOST_H
#define UAE_MHI_HOST_H

#include "uae/types.h"

struct TrapContext;

constexpr uae_u32 UAE_MHI_TRAP_ALLOC = 110;
constexpr uae_u32 UAE_MHI_TRAP_FREE = 111;
constexpr uae_u32 UAE_MHI_TRAP_QUEUE = 112;
constexpr uae_u32 UAE_MHI_TRAP_GET_EMPTY = 113;
constexpr uae_u32 UAE_MHI_TRAP_STATUS = 114;
constexpr uae_u32 UAE_MHI_TRAP_PLAY = 115;
constexpr uae_u32 UAE_MHI_TRAP_STOP = 116;
constexpr uae_u32 UAE_MHI_TRAP_PAUSE = 117;
constexpr uae_u32 UAE_MHI_TRAP_SET_PARAM = 118;
constexpr uae_u32 UAE_MHI_TRAP_QUERY = 119;

uae_u32 mhi_host_alloc(TrapContext *ctx, uaecptr task, uae_u32 sigmask);
uae_u32 mhi_host_free(TrapContext *ctx, uae_u32 handle);
uae_u32 mhi_host_queue(TrapContext *ctx, uae_u32 handle, uaecptr buffer, uae_u32 size, uae_u32 token);
uae_u32 mhi_host_get_empty(TrapContext *ctx, uae_u32 handle);
uae_u32 mhi_host_status(TrapContext *ctx, uae_u32 handle);
uae_u32 mhi_host_play(TrapContext *ctx, uae_u32 handle);
uae_u32 mhi_host_stop(TrapContext *ctx, uae_u32 handle);
uae_u32 mhi_host_pause(TrapContext *ctx, uae_u32 handle);
uae_u32 mhi_host_set_param(TrapContext *ctx, uae_u32 handle, uae_u32 param, uae_u32 value);
uae_u32 mhi_host_query(TrapContext *ctx, uae_u32 query);

#endif /* UAE_MHI_HOST_H */
