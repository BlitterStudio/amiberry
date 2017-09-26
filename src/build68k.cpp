/*
 * UAE - The Un*x Amiga Emulator
 *
 * Read 68000 CPU specs from file "table68k" and build table68k.c
 *
 * Copyright 1995,1996 Bernd Schmidt
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include <stdlib.h>
#include <tchar.h>
#include <assert.h>
#include <ctype.h>

#define TCHAR char

#include "readcpu.h"

static FILE *tablef;
static int nextch = 0;

static void getnextch(void)
{
    do {
	nextch = fgetc(tablef);
	if (nextch == '%') {
	    do {
		nextch = fgetc(tablef);
	    } while (nextch != EOF && nextch != '\n');
	}
    } while (nextch != EOF && isspace(nextch));
}

static int nextchtohex(void)
{
    switch (isupper (nextch) ? tolower (nextch) : nextch) {
     case '0': return 0;
     case '1': return 1;
     case '2': return 2;
     case '3': return 3;
     case '4': return 4;
     case '5': return 5;
     case '6': return 6;
     case '7': return 7;
     case '8': return 8;
     case '9': return 9;
     case 'a': return 10;
     case 'b': return 11;
     case 'c': return 12;
     case 'd': return 13;
     case 'e': return 14;
     case 'f': return 15;
     default: abort();
    }
}

int main(int argc, char **argv)
{
  int no_insns = 0;

  printf ("#include \"sysconfig.h\"\n");
  printf ("#include \"sysdeps.h\"\n");
  printf ("#include \"readcpu.h\"\n");
  printf ("struct instr_def defs68k[] = {\n");
  tablef = stdin;
  getnextch();
  while (nextch != EOF) {
	  int cpulevel, uncpulevel, plevel, sduse;
	  int i;

	  char opcstr[256];
	  int bitpos[16];
	  int flagset[5], flaguse[5];
	  char cflow;

	  unsigned int bitmask,bitpattern;
	  int n_variable;

	  int head = 0, tail = 0, clocks = 0, fetchmode = 0;

	  n_variable = 0;
	  bitmask = bitpattern = 0;
	  memset (bitpos, 0, sizeof(bitpos));
	  for(i=0; i<16; i++) {
	    int currbit;
	    bitmask <<= 1;
	    bitpattern <<= 1;

	    switch (nextch) {
        case '0': currbit = bit0; bitmask |= 1; break;
        case '1': currbit = bit1; bitmask |= 1; bitpattern |= 1; break;
        case 'c': currbit = bitc; break;
        case 'C': currbit = bitC; break;
        case 'f': currbit = bitf; break;
        case 'i': currbit = biti; break;
        case 'I': currbit = bitI; break;
        case 'j': currbit = bitj; break;
        case 'J': currbit = bitJ; break;
        case 'k': currbit = bitk; break;
        case 'K': currbit = bitK; break;
        case 's': currbit = bits; break;
        case 'S': currbit = bitS; break;
        case 'd': currbit = bitd; break;
        case 'D': currbit = bitD; break;
        case 'r': currbit = bitr; break;
        case 'R': currbit = bitR; break;
        case 'z': currbit = bitz; break;
        case 'E': currbit = bitE; break;
        case 'p': currbit = bitp; break;
        default: abort();
	    }
	    if (!(bitmask & 1)) {
		    bitpos[n_variable] = currbit;
		    n_variable++;
	    }

	    if (nextch == '0' || nextch == '1')
    		bitmask |= 1;
	    if (nextch == '1')
    		bitpattern |= 1;
	    getnextch();
  	}

  	while (isspace(nextch) || nextch == ':') /* Get CPU level, unimplemented level, and privilege level */
	    getnextch();

  	switch (nextch) {
      case '0': cpulevel = 0; break;
      case '1': cpulevel = 1; break;
      case '2': cpulevel = 2; break;
      case '3': cpulevel = 3; break;
      case '4': cpulevel = 4; break;
      case '5': cpulevel = 5; break;
      case '6': cpulevel = 6; break;
      case '7': cpulevel = 7; break;
      default: abort();
	  }
	  getnextch();

  	switch (nextch) {
      case '0': uncpulevel = 0; break;
      case '1': uncpulevel = 1; break;
      case '2': uncpulevel = 2; break;
      case '3': uncpulevel = 3; break;
      case '4': uncpulevel = 4; break;
      case '5': uncpulevel = 5; break;
      case '6': uncpulevel = 6; break;
      case '7': uncpulevel = 7; break;
      default: abort();
	  }
	  getnextch();

  	switch (nextch) {
      case '0': plevel = 0; break;
      case '1': plevel = 1; break;
      case '2': plevel = 2; break;
      case '3': plevel = 3; break;
      default: abort();
	  }
	  getnextch();

  	while (isspace(nextch))                   /* Get flag set information */
      getnextch();

	  if (nextch != ':')
      abort();

	  for(i = 0; i < 5; i++) {
      getnextch();
      switch(nextch) {
        case '-': flagset[i] = fa_unset; break;
        case '/': flagset[i] = fa_isjmp; break;
        case '+': flagset[i] = fa_isbranch; break;
        case '0': flagset[i] = fa_zero; break;
        case '1': flagset[i] = fa_one; break;
        case 'x': flagset[i] = fa_dontcare; break;
        case '?': flagset[i] = fa_unknown; break;
        default: flagset[i] = fa_set; break;
      }
    }

	  getnextch();
	  while (isspace(nextch))
      getnextch();

	  if (nextch != ':')                        /* Get flag used information */
      abort();

	  for(i = 0; i < 5; i++) {
      getnextch();
      switch(nextch) {
	      case '-': flaguse[i] = fu_unused; break;
	      case '/': flaguse[i] = fu_isjmp; break;
	      case '+': flaguse[i] = fu_maybecc; break;
	      case '?': flaguse[i] = fu_unknown; break;
	      default: flaguse[i] = fu_used; break;
	    }
	  }

	  getnextch();
	  while (isspace(nextch))
      getnextch();

	  if (nextch != ':')                        /* Get control flow information */
      abort();
	
	  cflow = 0;
	  for(i = 0; i < 2; i++) {
		  getnextch();
		  switch(nextch) {
		    case '-': break;
		    case 'R': cflow |= fl_return; break;
		    case 'B': cflow |= fl_branch; break;
		    case 'J': cflow |= fl_jump; break;
		    case 'T': cflow |= fl_trap; break;
		    default: abort();
		  }
	  }
	
	  getnextch();
	  while (isspace(nextch))
      getnextch();

	  if (nextch != ':')                        /* Get source/dest usage information */
      abort();

	  getnextch();
	  sduse = nextchtohex() << 4;
	  getnextch();
	  sduse |= nextchtohex();

	  getnextch();
	  while (isspace(nextch))
      getnextch();

	  if (nextch != ':')
      abort();

	  if (fgets(opcstr, 250, tablef) != opcstr) {
		  abort();
	  }
	  getnextch();

	  if (nextch == '-') {
		  int neg;
		  char fm[20];
		  getnextch();
		  while (isspace(nextch))
			  getnextch();
		  neg = 1;
		  if (nextch == '-') {
			  neg = -1;
			  getnextch();
		  }
		  for (;;) {
			  if (nextch < '0' || nextch > '9')
				  break;
			  head *= 10;
			  head += nextch - '0';
			  nextch = fgetc (tablef);
		  }
		  head *= neg;
		  while (isspace(nextch))
			  getnextch();
		  for (;;) {
			  if (nextch < '0' || nextch > '9')
				  break;
			  tail *= 10;
			  tail += nextch - '0';
			  nextch = fgetc (tablef);
		  }
		  while (isspace(nextch))
			  getnextch();
		  for (;;) {
			  if (nextch < '0' || nextch > '9')
				  break;
			  clocks *= 10;
			  clocks += nextch - '0';
			  nextch = fgetc (tablef);
		  }
		  if (nextch == ' ') {
			  if (fgets(fm, sizeof fm, tablef) != fm) {
				  abort();
			  }
			  if (!strnicmp(fm, "fea", 3))
				  fetchmode = 1;
			  if (!strnicmp(fm, "cea", 3))
				  fetchmode = 2;
			  if (!strnicmp(fm, "fiea", 4))
				  fetchmode = 3;
			  if (!strnicmp(fm, "ciea", 4))
				  fetchmode = 4;
			  if (!strnicmp(fm, "jea", 3))
				  fetchmode = 5;
		  }
		  getnextch();
	  }

    int j;
    /* Remove superfluous spaces from the string */
    char *opstrp = opcstr, *osendp;
    char tmp[100], *p;
    int slen = 0;

    while (isspace((int)*opstrp))
		  opstrp++;

    osendp = opstrp;
    while (*osendp) {
		  if (!isspace ((int)*osendp))
	      slen = osendp - opstrp + 1;
		  osendp++;
    }
    opstrp[slen] = 0;

    if (no_insns > 0)
		  printf(",\n");
    no_insns++;
    strcpy (tmp, opstrp);
    strcat (tmp, " ");
    p = tmp;
    while (!isspace(*p++));
      *p = 0;
    printf("/* %s */\n", tmp);
    printf("{0x%04X,%2d,{", bitpattern, n_variable);
    for (j = 0; j < 16; j++) {
		  printf("%2d", bitpos[j]);
		  if (j < 15)
		      printf(",");
    }
    printf ("},0x%04X,%d,%d,%d,{", bitmask, cpulevel, uncpulevel, plevel);
    for(i = 0; i < 5; i++) {
		  printf("{%d,%d}%s", flaguse[i], flagset[i], i == 4 ? "" : ",");
    }
    printf("}, %2d, %2d,_T(\"%s\"),%2d,%2d,%2d,%2d}", cflow, sduse, opstrp, head, tail, clocks, fetchmode);
  }
  printf("};\nint n_defs68k = %d;\n", no_insns);
  return 0;
}
