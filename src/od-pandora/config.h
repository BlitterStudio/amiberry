 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * User configuration options
  *
  * Copyright 1995 - 1998 Bernd Schmidt
  */

/*
 * Please note: Many things are configurable with command line parameters,
 * and you can put anything you can pass on the command line into a 
 * configuration file ~/.uaerc. Please read the documentation for more
 * information.
 * 
 * NOTE NOTE NOTE
 * Whenever you change something in this file, you have to "make clean"
 * afterwards.
 * Don't remove the '#' signs. If you want to enable something, move it out
 * of the C comment block, if you want to disable something, move it inside
 * the block.
 */

/*
 * When USE_COMPILER is defined, a m68k->i386 instruction compiler will be
 * used. This is experimental. It has only been tested on a Linux/i386 ELF
 * machine, although it might work on other i386 Unices.
 * This is supposed to speed up application programs. It will not work very
 * well for hardware bangers like games and demos, in fact it will be much
 * slower. It can also be slower for some applications and/or benchmarks.
 * It needs a lot of tuning. Please let me know your results with this.
 * The second define, RELY_ON_LOADSEG_DETECTION, decides how the compiler 
 * tries to detect self-modifying code. If it is not set, the first bytes
 * of every compiled routine are used as checksum before executing the
 * routine. If it is set, the UAE filesystem will perform some checks to 
 * detect whether an executable is being loaded. This is less reliable
 * (it won't work if you don't use the harddisk emulation, so don't try to
 * use floppies or even the RAM disk), but much faster.
 *
 * @@@ NOTE: This option is unfortunately broken in this version. Don't
 * try to use it. @@@
 *
#define USE_COMPILER
#define RELY_ON_LOADSEG_DETECTION
 */

/***************************************************************************
 * Operating system/machine specific options
 * Configure these for your CPU. The default settings should work on any
 * machine, but may not give optimal performance everywhere.
 * (These don't do very much yet, except HAVE_RDTSC
 */
