#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>


#define MAX_TRACE 2000

static void* trc_func[MAX_TRACE];
static void* trc_caller[MAX_TRACE];
static int trc_enter[MAX_TRACE];
static int trc_number[MAX_TRACE];
static int trc_next_write = 0;
static int trc_counter = 0;

static int do_trace = 0;

static FILE *fd;


void trace_begin (void)
{
  if(do_trace)
    return;
  memset(trc_enter, 0, sizeof(int) * MAX_TRACE);
  do_trace = 1;
}
 
void trace_end (void)
{
  if(do_trace) {
    do_trace = 0;

    fd = fopen("trace.txt", "w");
    for(int i=0; i<MAX_TRACE; ++i) {
      if(trc_enter[trc_next_write] > 0) {
      	Dl_info dlinfo;
        memset(&dlinfo, 0, sizeof(dlinfo));
        int func_found = dladdr(trc_func[trc_next_write], &dlinfo);
        if(func_found && dlinfo.dli_sname != NULL) {
          fprintf(fd, "%8d - %s 0x%08X from 0x%08X (%s)\n", trc_number[trc_next_write], (trc_enter[trc_next_write] == 1 ? "enter" : "leave"),
            trc_func[trc_next_write], trc_caller[trc_next_write], dlinfo.dli_sname);
        }
      }
      ++trc_next_write;
      if(trc_next_write >= MAX_TRACE)
        trc_next_write = 0;
    }
    fclose(fd);
  }
}
 
void __cyg_profile_func_enter (void *func,  void *caller)
{
  if(do_trace) {
    trc_enter[trc_next_write] = 1;
    trc_func[trc_next_write] = func;
    trc_caller[trc_next_write] = caller;
    trc_number[trc_next_write] = trc_counter;
    ++trc_counter;
    ++trc_next_write;
    if(trc_next_write >= MAX_TRACE)
      trc_next_write = 0;
  }
}
 
void __cyg_profile_func_exit (void *func, void *caller)
{
  if(do_trace) {
    trc_enter[trc_next_write] = 2;
    trc_func[trc_next_write] = func;
    trc_caller[trc_next_write] = caller;
    trc_number[trc_next_write] = trc_counter;
    ++trc_counter;
    ++trc_next_write;
    if(trc_next_write >= MAX_TRACE)
      trc_next_write = 0;
  }
}
