/*
 * PC cassette interface.
 *
 * The IBM PC and PCjr feed PIT channel 2 to the cassette port. WinUAE's
 * bridgeboards have no cassette, so 86box_glue.cpp keeps the cassette
 * pointer NULL and pc_cas_set_out() empty; the PIT code only needs the
 * declarations.
 *
 * Copyright (C) 2026 Stefan Reinauer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef EMU_CASSETTE_H
#define EMU_CASSETTE_H

typedef struct pc_cassette_t pc_cassette_t;

extern pc_cassette_t *cassette;

void pc_cas_set_out(pc_cassette_t *cas, unsigned char val);

#endif /* EMU_CASSETTE_H */
