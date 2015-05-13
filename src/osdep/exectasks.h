 /*
  * UAE - The Un*x Amiga Emulator
  *
  * exec.library multitasking emulation
  *
  * Copyright 1996 Bernd Schmidt
  */

struct switch_struct {
    int dummy;
};
/* Looks weird. I think I had a report once that some compiler chokes if
 * the statement is empty. */
#define EXEC_SWITCH_TASKS(run, ready) do { int i = 0; i++; } while(0)

#define EXEC_SETUP_SWS(t) do { int i = 0; i++; } while(0)

