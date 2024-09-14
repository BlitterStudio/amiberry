


/********************************************************/
/* Initialise the DSP emulator                          */
/*                                                      */
/********************************************************/
void DSP_init_emu ();

/********************************************************/
/* Shut down the DSP emulator                           */
/*                                                      */
/********************************************************/
void DSP_shutdown_emu ();

/********************************************************/
/* Reset the DSP                                        */
/*                                                      */
/********************************************************/
void DSP_reset();
//note this is triggered via bit 7 of 0xDD0080 going from 1 to 0

/********************************************************/
/* Trigger an interrupt level 0                         */
/*                                                      */
/********************************************************/
bool DSP_int0();
bool DSP_int0_masked();
//note this is triggered via bit 0 of 0xDD0080 going from 1 to 0

/********************************************************/
/* Trigger an interrupt level 1                         */
/*                                                      */
/********************************************************/
bool DSP_int1();
bool DSP_int1_masked();
//note this is triggered via bit 1 of 0xDD0080 going from 1 to 0

/********************************************************/
/* DSP has changed M68k interrupt line                  */
/*                                                      */
/********************************************************/
void DSP_external_interrupt(int32_t level, int32_t state);
//state is the state of the associated line, 0 triggers an M68k interrupt
//level=0 corresponds to INT2, level=1 corresponds to INT6
//this routine needs to mask interrupts 

/********************************************************/
/* Execute a single DSP instruction, allowing for       */
/* instruction and memory latencies                     */
/********************************************************/
void DSP_execute_insn ();

/********************************************************/
/* Read values from the Amiga's memory space            */
/*                                                      */
/********************************************************/

uint32_t DSP_get_long_ext(uint32_t addr);
uint16_t DSP_get_short_ext(uint32_t addr);
unsigned char DSP_get_char_ext(uint32_t addr);


/********************************************************/
/* Set values in the Amiga's memory space               */
/*                                                      */
/********************************************************/

void DSP_set_long_ext(uint32_t addr, uint32_t value);
void DSP_set_short_ext(uint32_t addr, uint16_t value);
void DSP_set_char_ext(uint32_t addr, unsigned char value);

extern char DSP_irsh_flag;

//Notes:
//The DSP write register is byte sized and located at 0xDD0080
//The DSP read register (which should mirror the write register)
//is long word sized and located at 0xDD005C
//
//For more details on the Amiga interface to the DSP3210, see here: 
//http://www.devili.iki.fi/mirrors/haynie/research/a3000p/docs/dspprg.pdf