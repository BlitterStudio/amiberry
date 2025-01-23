
#include "stdlib.h"
#include "stdint.h"
#include "math.h"

#include "DSP3210_emulation.h"

extern void write_log(const char *, ...);
#ifdef DEBUGGER
extern void activate_debugger(void);
#endif

/* Typedefs */
typedef union 
{
	int32_t i;
	float f;

} intfloat;

/* Variable declarations */
static int32_t latency3[2][2];		//ticks to execution, PC address
static int32_t K_val = -1;
static int32_t loopcount = 0;
static int32_t loopstart = 0;
static int32_t bio6, bio7;			//for M68k interrupts

//externally used
int32_t DSP_reg_r[25];
int32_t DSP_io_r[16];
int32_t DSP_reg_irsh;		//shadow instruction register
char DSP_irsh_flag;			//1 indicates instruction in irsh to be executed
int32_t DSP_irsh_addr;
int32_t DSP_irsh_pcw;
float DSP_reg_a[4];
float DSP_history_a[5][5];
uint16_t DSP_history_CC[5];
int32_t DSP_history_mem[5][5];
uint8_t *DSP_onboard_RAM;
extern uint8_t DSP_onboard_ROM[];
uint8_t *DSP_MMIO;

/* Defines */
#define CC DSP_io_r[0]
#define reg_evtp DSP_reg_r[22]
#define reg_PC DSP_reg_r[23]
#define reg_pcsh DSP_reg_r[24]
#define reg(val) DSP_reg_r[DSP_regno(val)]
#define regS(val) (int16_t)(reg(val) & 0xFFFF)
#define io(val) DSP_io_r[DSP_regno(val)]
#define io_emr DSP_io_r[8]		//16 bit
#define io_pcw DSP_io_r[12]		//16 bit
#define io_dauc DSP_io_r[14]	//8 bit

//CAU field definitions
#define Wfield(inst) (inst&0x00E00000)>>21
#define RPfield(inst) (inst&0xF800)>>11
#define RIfield(inst) (inst&0x1F)
#define RHfield(inst) (inst&0x1F0000)>>16
#define RDfield(inst) (inst&0x03E00000)>>21
#define RSfield(inst) (inst&0x001F0000)>>16
#define Nfield(inst) (inst&0xFFFF)
#define NEfield(inst) (inst&0x1FE00000)>>21
#define FCAUfield(inst) (inst&0x01E00000)>>21
#define Cfield(inst) (inst&0x000007E0)>>5
#define Cgotofield(inst) (inst&0x07E00000)>>21
#define RLfield(inst) Nfield(inst)
#define RS1field(inst) RPfield(inst)
#define RS2field(inst) RIfield(inst)
#define RS3field(inst) RSfield(inst)
#define RBfield(inst) RSfield(inst)
#define RMfield(inst) RDfield(inst)

//DAU field definitions
#define Mfield(inst) (inst&0x1C000000)>>26
#define Ffield(inst) (inst&0x01000000)>>24
#define Sfield(inst) (inst&0x00800000)>>23
#define NDAUfield(inst) (inst&0x00600000)>>21
#define Xfield(inst) (inst&0x001FC000)>>14
#define Yfield(inst) (inst&0x00003F80)>>7
#define Zfield(inst) (inst&0x0000007F)
#define Gfield(inst) (inst&0x07800000)>>23

#define FLOAT_TO_INT(x) ((x)>=0?(int32_t)((x)+0.5):(int32_t)((x)-0.5))
#define Ssign(x) (Sfield(x)==0?1:-1)
#define Fsign(x) (Ffield(x)==0?1:-1)

//condition codes
#define CCn 1
#define CCz 2
#define CCv 4
#define CCc 8
#define CCN 16
#define CCZ 32
#define CCU 64
#define CCV 128

#define CCnf (CC&CCn)
#define CCzf (CC&CCz)
#define CCvf (CC&CCv)
#define CCcf (CC&CCc)
#define CCNf (CC&CCN)
#define CCZf (CC&CCZ)
#define CCUf (CC&CCU)
#define CCVf (CC&CCV)

//latency timing
#define DSP_reg_atime 2

//for 64 bit manipulations
#define hi32 ~(int64_t)0xFFFFFFFF
#define bt32  (int64_t)0x80000000
#define hi48 ~(int64_t)0xFFFF
#define bt16  (int64_t)0x8000
static const double max_pos_float = (2-2^-31)*2^127;
static const double max_neg_float = -2*2^127;
static const double min_pos_float = 2^-127;
static const double min_neg_float = (-1+2^-31)*2^-127;

//internal ROM/RAM locations
#define map0_ROM_start 0x50030000
#define map0_ROM_end (map0_ROM_start + 0x3FF)
#define map0_RAM_start (map0_ROM_start + 0xE000)
#define map0_RAM_end (map0_ROM_start + 0xFFFF)
#define map0_MMIO_start (map0_ROM_start + 0x400)
#define map0_MMIO_end (map0_ROM_start + 0x7FF)
#define map1_ROM_start 0
#define map1_ROM_end (map1_ROM_start + 0x3FF)
#define map1_RAM_start (map1_ROM_start + 0xE000)
#define map1_RAM_end (map1_ROM_start + 0xFFFF)
#define map1_MMIO_start (map1_ROM_start + 0x400)
#define map1_MMIO_end (map1_ROM_start + 0x7FF)

/* Function declarations */
static void doinst(int32_t addr);
static int32_t DAUXYZ(int32_t incode, int32_t field, int32_t *fld2);
static int32_t GetXYfield(int32_t fld1,int32_t fld2,int32_t incode);
static void DoLat3(int32_t pcval);
static void SetZfield(int32_t fld1, int32_t fld2, int32_t temp);
static void doDAU5(int32_t incode);
static float Mval(int32_t incode);
static float SetCC(double dtemp);
static int32_t SetCCCAU(int64_t res, int32_t input);
static int32_t SetCCCAU16(int64_t res, int32_t input);
static void SetCCnzCAU(int32_t res);
static int32_t TestCfield(int32_t code);
static float round_dsp(float num);
static int32_t bitreverse(int32_t n, int32_t bits);
static void DSP_exception_illegal();
static void DSP_exception_UOflow();
static void DSP_exception_NaN();
static void DSP_undefined();
static void DSP_breakpoint();
static void DSP_check_interrupt(int32_t value, int32_t bitno);

//externally used
int32_t DSP_swapl(int32_t a);
int16_t DSP_swapw(int16_t a);
int32_t DSP_regno(int32_t r);
int32_t DSP_float(float f);
int32_t DSP_negfloat(int32_t f);
int32_t DSP_dsp2ieee(int32_t f);
int32_t DSP_get_long(int32_t addr);
void DSP_set_long(int32_t addr, int32_t value);
int16_t DSP_get_short(int32_t addr);
void DSP_set_short(int32_t addr, int16_t value);
char DSP_get_char(int32_t addr);
void DSP_set_char(int32_t addr, char value);

/********************************************************/
/* Initialise the DSP emulator                          */
/*                                                      */
/********************************************************/
void DSP_init_emu ()
{
	DSP_onboard_RAM = (uint8_t*)malloc(8192);	//8k onboard RAM
	DSP_MMIO = (uint8_t*)malloc(0x800);
	reg_PC = 0;
	return;
}

/********************************************************/
/* Shut down the DSP emulator                           */
/*                                                      */
/********************************************************/
void DSP_shutdown_emu ()
{
	free(DSP_MMIO);
	free(DSP_onboard_RAM);
	DSP_MMIO = NULL;
	DSP_onboard_RAM = NULL;
	return;
}

/********************************************************/
/* Trigger DSP reset                                    */
/*                                                      */
/********************************************************/
void DSP_reset()
{
	int32_t i;
	for(i=0; i<22; i++) DSP_reg_r[i]=0;
	for(i=0; i<4; i++) DSP_reg_a[i]=0;
	latency3[0][0]=0;
	latency3[1][0]=0;
	reg_pcsh = 0;

	//The Information Manual only mentions the following registers being initialised
	reg_evtp = 0;
	DSP_reg_r[20] = reg_PC;
	reg_PC = 0;
	io_dauc = 0;
	DSP_io_r[12] = 0x0F8F;	//0b0000111110001111; bio 4,5 = 0 , bio 6,7 = 1, set pcw reg
	io_emr = 0;

	//other MMUIO regs are set here too - see 7-11
	bio6 = 1;
	bio7 = 1;
	DSP_irsh_flag = 0;
	return;
}

bool DSP_int0_masked()
{
	return (io_emr & 0x0100) == 0;
}

bool DSP_int1_masked()
{
	return (io_emr & 0x8000) == 0;
}

/********************************************************/
/* Trigger an interrupt level 0                         */
/*                                                      */
/********************************************************/
bool DSP_int0()
{
	if(!(io_emr & 0x0100))
		return false;	//return if int0 masked out
	
	//save chip state: DO loop status, a0-a3, dauc
	//not implemented yet
	reg_pcsh = reg_PC;	//save PC
	reg_PC = reg_evtp + 8*8;	//INT0 entry
	DSP_irsh_flag = 2;	//no more waiting, execute interrupt
	return true;
}

/********************************************************/
/* Trigger an interrupt level 1                         */
/*                                                      */
/********************************************************/
bool DSP_int1()
{
	if(!(io_emr & 0x8000))
		return false;	//return if int1 masked out

	//save chip state: DO loop status, a0-a3, dauc
	//not implemented yet
	reg_pcsh = reg_PC;	//save PC
	reg_PC = reg_evtp + 8*15;	//INT1 entry
	DSP_irsh_flag = 2;	//no more waiting, execut interrupt
	return true;
}

/********************************************************/
/* DSP reaches a breakpoint                             */
/*                                                      */
/********************************************************/
void DSP_breakpoint()
{
	//It's unspecified in the DSP3210 Instruction Manual
	//what actually happens on a 'bkpt' instruction
	//In fact, the instruction isn't even listed in the
	//instruction set! It only appears in the listing for
	//the ROM code in Appendix A

	return;
}

/********************************************************/
/* Execute a single DSP instruction, allowing for       */
/* instruction and memory latencies                     */
/********************************************************/

void DSP_execute_insn () 
{
	int32_t i,j;
	DSP_history_mem[2][0] = 0;	//no mem to write by default
	doinst(reg_PC);

	//update history of mem writes - latency 1
	for(j=4;j>0;j--) {
		DSP_history_mem[0][j] = DSP_history_mem[0][j-1];	//location
		DSP_history_mem[1][j] = DSP_history_mem[1][j-1];	//value
		DSP_history_mem[2][j] = DSP_history_mem[2][j-1];	//flag for delayed write
	}
	switch(DSP_history_mem[2][4])
	{
	case 1:
		//do write
		//*(int32_t *)DSP_history_mem[0][4]=DSP_history_mem[1][4];	//dummy
		DSP_set_long(DSP_history_mem[0][4], DSP_history_mem[1][4]);
		break;

	case 2:
		//*(int16_t *)DSP_history_mem[0][4]=DSP_history_mem[1][4];	//dummy
		DSP_set_short(DSP_history_mem[0][4], DSP_history_mem[1][4]);
		break;

	default:
		break;
	}

	//update history of a registers - latency 2
	for(i=0;i<4;i++) { //step through registers
		for(j=4;j>0;j--) 
			DSP_history_a[i][j] = DSP_history_a[i][j-1]; //age each entry
		DSP_history_a[i][0] = DSP_reg_a[i];	//insert new aN values
	}

	//update history of CC - latency 4
	for(j=4;j>0;j--)
		DSP_history_CC[j] = DSP_history_CC[j-1];
	DSP_history_CC[0] = CC;
	DSP_io_r[15] = ((DSP_io_r[15] << 1) | (CCNf >> 4)) &0x3F;	//update ctr

	//deal with latency 3 - delayed jumps
	if(latency3[0][0])
	{		//we have a latent jump in the queue
		if(latency3[0][0]--==1)		//decrease counter
			reg_PC = latency3[0][1];		//process the jump
	}
	if(latency3[1][0])
	{	//latent jump in 2nd slot
		if(latency3[1][0]--==1)
			reg_PC = latency3[1][1];	//process the jump
	}

	//do loop
	if(reg_PC == K_val) {
		loopcount--;
		if(loopcount>=0) reg_PC = loopstart; 
	}
}

/********************************************************/
/* Main code to execute a single DSP instruction        */
/********************************************************/

void doinst(int32_t addr)
{
	int32_t valX, valY, fld1, fld2, fld3, fld4,value,tempc,pinc;
	float ftemp,fvalX,fvalY;
	float delayvalX, delayvalY, delayM;
	double dtemp;
	int64_t val64;
	intfloat conv;
	int32_t incode;
	int32_t baseaddr;

	if(DSP_irsh_flag == 3) return;			//nothing to do while in waiti
	if(DSP_irsh_flag == 4) DSP_irsh_flag -= 1;	//execute one more instruction
	//int32_t incode = swapl(*(amiga_mem_base + addr));
	if(DSP_irsh_flag == 1) {
		DSP_irsh_flag = 0;
		incode = DSP_reg_irsh;
		reg_PC = reg_PC - 4;	//undo the PC increment coming up
	} else {
		//DSP_irsh_flag -= 1;
		incode = DSP_swapl(DSP_get_long(addr));
	}

	switch((incode & 0xE0000000)>>29)
	{
	case 1:
	case 2:
	case 3:
		if((incode & 0xF8000000)>>27 != 15) {
			//do some preliminaries if DAU 1-4
			fld1=DAUXYZ(incode,1,&fld2);	//get Xfield values
			valX=GetXYfield(fld1,fld2,incode);		//read value from Xfield reg and post inc
			fld3=DAUXYZ(incode,2,&fld4);	//get Yfield values
			valY=GetXYfield(fld3,fld4,incode);		//read value from Yfield reg and post inc

			//convert valX,Y to floats
			//*(int32_t *)&fvalX=swapl(valX);	//'''
			conv.i = DSP_swapl(valX);
			fvalX = conv.f;

			//*(int32_t *)&fvalY=swapl(valY);
			conv.i = DSP_swapl(valY);
			fvalY = conv.f;

			//factor in delayed a reg updates
			if(!fld1&&fld2<4) delayvalX = DSP_history_a[fld2][DSP_reg_atime]; else delayvalX = fvalX;
			if(!fld3&&fld4<4) delayvalY = DSP_history_a[fld4][DSP_reg_atime]; else delayvalY = fvalY;

			if(Mfield(incode)<4) delayM = DSP_history_a[Mfield(incode)][DSP_reg_atime]; else delayM = Mval(incode);
			//can now use delayvals as multiplier inputs
		} 
		break;
	}

	reg_PC=reg_PC+4;	
	switch((incode & 0xE0000000)>>29)
	{
	case 1:
		//DAU 1 or 4 instruction
		//if(Mfield(incode)!=6)
		switch(Mfield(incode))
		{
		case 6: //DAU4
			dtemp = Fsign(incode)*fvalY+Ssign(incode)*fvalX;
			ftemp = SetCC(dtemp);
			DSP_reg_a[NDAUfield(incode)]=ftemp;	//set aN to value
			value = valY;	//Zfield will be set to Yfield, if required

			fld1=DAUXYZ(incode,3,&fld2);	//get Zfield values
			SetZfield(fld1, fld2, DSP_swapl(value));	//and set Zfield if needed	
			break;

		case 7:
			DSP_exception_illegal();
			break;

		default:
			//DAU1
			dtemp = Fsign(incode)*fvalY+Ssign(incode)*delayM*delayvalX;
			ftemp = SetCC(dtemp);
			DSP_reg_a[NDAUfield(incode)]=ftemp;	//set aN to value
			//value = *(int32_t *)&ftemp;	//convert to int '''
			conv.f = ftemp;
			value = conv.i;

			fld1=DAUXYZ(incode,3,&fld2);	//get Zfield values
			SetZfield(fld1, fld2, DSP_swapl(value));	//and set Zfield if needed
			break;
		}			
		break;

	case 2:
		//DAU 2 instruction
		switch(Mfield(incode))
		{
		case 6:
		case 7:
			DSP_exception_illegal();
			break;

		default:
			dtemp = Fsign(incode)*Mval(incode)+Ssign(incode)*delayvalY*delayvalX;
			ftemp = SetCC(dtemp);
			DSP_reg_a[NDAUfield(incode)]=ftemp;	//set aN to value
			value = valY;	//Zfield will be set to Yfield, if required

			fld1=DAUXYZ(incode,3,&fld2);	//get Zfield values
			SetZfield(fld1, fld2, DSP_swapl(value));	//and set Zfield if needed
			break;
		}
		break;

	case 3:
		//DAU 3 or 5 instruction
		if(Mfield(incode)!=6 && Mfield(incode)!=7)
		{		//DAU 3
			dtemp = Fsign(incode)*Mval(incode)+Ssign(incode)*delayvalY*delayvalX;
			ftemp = SetCC(dtemp);
			DSP_reg_a[NDAUfield(incode)]=ftemp;	//set aN to value
			//value = *(int32_t *)&ftemp;	//convert to int '''
			conv.f = ftemp;
			value = conv.i;

			fld1=DAUXYZ(incode,3,&fld2);	//get Zfield values
			SetZfield(fld1, fld2, DSP_swapl(value));	//and set Zfield if needed
		} else { //DAU 5
			doDAU5(incode);
		}
		break;

	case 5:
		//Unconditional branch instruction GOTO rB+M
		DoLat3((uint32_t)((NEfield(incode)<<16)|Nfield(incode))+4);
		//latency 3!
		break;

	case 6:
		//24 bit immediate load instruction SET24
		reg(RSfield(incode)) = (uint32_t)((NEfield(incode)<<16)|Nfield(incode));
		break;

	case 7:
		//call 24-bit immediate instruction CALL M,(rM)
		reg(RSfield(incode)) = reg_PC+4;	//return to the instruction after the latent one
		DoLat3((uint32_t)((NEfield(incode)<<16)|Nfield(incode))+4);
		//latency 3!
		break;

	default:
		switch((incode&0xF8000000)>>27)
		{
		case 0:	//reserved
			DSP_exception_illegal();
			break;

		case 16:
			//conditional branch
			switch(incode)
			{
            case static_cast<int32_t>(0x80000000):
				//nop
				reg_PC = reg_PC + 0;
				break;

            case static_cast<int32_t>(0x803E0000):
				//ireturn
				DSP_irsh_flag = 1;
				reg_PC = reg_pcsh;	//jump execution to shadown PC
				break;

			default:
				if(TestCfield(Cgotofield(incode)))
				{
					DoLat3(reg(RBfield(incode))+(int16_t)Nfield(incode) + ((RBfield(incode)==15)?4:0));
					//if(RBfield(incode)==15) reg_PC+=4;	//account for extra instr in pipeline
				}
				//latency 3!
				break;
			}
			break;

		default:
			switch((incode&0xFC000000)>>26)
			{
			case 2:		//reserved
			case 34:	//reserved
				DSP_exception_illegal();
				break;

			case 3:
				//loop counter instruction GOTO-LOOP
				reg(RMfield(incode))-=1;
				if(reg(RMfield(incode))>=0)
				{
					DoLat3(reg(RBfield(incode)) + (int16_t)Nfield(incode) + ((RBfield(incode)==15)?4:0));
					//if(RBfield(incode)==15) reg_PC+=4;	//account for extra instr in pipeline
				}
				break;
				//latency 3!!

			case 35:
				//DO loops
				K_val = ((incode&0x3F800)>>9) + reg_PC + 4;
				loopstart = reg_PC;
				if((incode&0x02000000)>>25)
					//reg
					loopcount = reg(RS2field(incode));
				else
					//immediate
					loopcount = incode&0x7FF;
				break;

			case 4:
				//call reg with 16 bit N instruction CALL rB+N,(rM)
				reg(RMfield(incode)) = reg_PC+4;	//return to the instruction after the latent one
				DoLat3(reg(RSfield(incode)) + (int16_t)Nfield(incode) + ((RSfield(incode)==15)?4:0));
				//if(RSfield(incode)==15) reg_PC+=4;	//account for extra instr in pipeline
				//latency 3!
				break;

			case 36:
				//SHIFT-OR
				value = reg(RSfield(incode)) | (Nfield(incode)<<16);
				reg(RDfield(incode)) = value;
				SetCCnzCAU(value);
				break;

			case 5:
				//16-bit 3 operand add instruction ADD
				value = reg(RS3field(incode));
				val64 = (int64_t)value + (int64_t)((int16_t)Nfield(incode));
				if(RS3field(incode)==15)	//PC
					val64+=4;				//fudge
				reg(RDfield(incode)) = SetCCCAU(val64,value);
				break;

			case 37:
				//32-bit 3 operand add instruction ADD
				value = reg(RS3field(incode));
				val64 = (int64_t)value + (int64_t)Nfield(incode);
				if(RS3field(incode)==15)	//PC
					val64+=4;				//fudge
				reg(RDfield(incode)) = SetCCCAU(val64,value);

				break;

			case 6:
				//16-bit ALU instruction
				if((incode&0x02000000)>>25)	//indicates immediate
				{
					switch(FCAUfield(incode))
					{
					case 7:	//compare
						value = regS(RBfield(incode));
						val64 = (int64_t)value - (int64_t)((int16_t)Nfield(incode));
						value = SetCCCAU16(val64,value);
						break;

					case 15: //and, no store
						value = regS(RBfield(incode))&((int16_t)Nfield(incode));
						SetCCnzCAU(value);
						break;

					case 1: //left shift
						value = regS(RBfield(incode)) << (int16_t)Nfield(incode);
						reg(RBfield(incode)) = value&0xFFFF;
						SetCCnzCAU(value);
						break;

					case 12: //logical right shift
						value = (int32_t)(((uint32_t)regS(RBfield(incode))) >> (int16_t)Nfield(incode));
						reg(RBfield(incode)) = value&0xFFFF;
						SetCCnzCAU(value);
						break;

					case 13: //arithmetic right shift
						value = regS(RBfield(incode)) >> (int16_t)Nfield(incode);
						reg(RBfield(incode)) = value&0xFFFF;
						SetCCnzCAU(value);
						break;

					case 2: // subtract N-rD
						value = (int16_t)Nfield(incode);
						val64 = (int64_t)value - (int64_t)regS(RBfield(incode));
						reg(RBfield(incode)) = SetCCCAU16(val64,value);
						break;

					case 4: // subtract rD-N
						value = regS(RBfield(incode));
						val64 = (int64_t)value - (int64_t)((int16_t)Nfield(incode));
						reg(RBfield(incode)) = SetCCCAU16(val64,value);
						break;

					case 3: // reverse carry add
						value = (int16_t)bitreverse(regS(RBfield(incode)), 16);
						value += (int16_t)bitreverse((int32_t)Nfield(incode), 16);
						value = (int16_t)bitreverse(value, 16);
						reg(RBfield(incode)) = value;
						SetCCnzCAU(value);
						break;

					case 6: //and with complement
					case 9:	//rotate right
					case 11: //rotate left
						//not with immediate operand
						DSP_undefined();
						break;

					case 8: //exclusive or
						value = regS(RBfield(incode))^(int16_t)Nfield(incode);
						reg(RBfield(incode)) = value;
						SetCCnzCAU(value);
						break;

					case 10: //or
						value = regS(RBfield(incode))|(int16_t)Nfield(incode);
						reg(RBfield(incode)) = value;
						SetCCnzCAU(value);
						break;

					case 14: //and
						value = regS(RBfield(incode))&(int16_t)Nfield(incode);
						reg(RBfield(incode)) = value;
						SetCCnzCAU(value);
						break;

					default:	//Fcode 5 is reserved
						DSP_undefined();
						break;
					}
				} else {	//Reg source
					if(TestCfield(Cfield(incode)))	//ie not always false
					{
						switch(FCAUfield(incode))
						{
						case 7:	//compare
							value = regS(RS1field(incode));
							val64 = (int64_t)value - (int64_t)regS(RS2field(incode));
							value = SetCCCAU16(val64, value);
							break;

						case 15: //and, no store
							value = regS(RS1field(incode))&regS(RS2field(incode));
							SetCCnzCAU(value);
							break;

						case 0: //add
							value = regS(RS1field(incode));
							val64 = (int64_t)value + (int64_t)regS(RS2field(incode));
							reg(RBfield(incode)) = SetCCCAU16(val64, value);
							break;
						
						case 1: //left shift
							//no test of RS2>26
							//unclear what happens if RS2=22!
							if(RS2field(incode)==23)	//+n
								value = regS(RS1field(incode))<<1;
							else
								value = regS(RS1field(incode))<<reg(RS2field(incode));
							reg(RBfield(incode)) = value&0xFFFF;
							SetCCnzCAU(value);
							break;

						case 12: //logical right shift
							if(RS2field(incode)==23)	//+n
								value = (int32_t)(((uint32_t)regS(RS1field(incode)))>>1);
							else
								value = (int32_t)(((uint32_t)regS(RS1field(incode)))>>reg(RS2field(incode)));
							reg(RBfield(incode)) = value&0xFFFF;
							SetCCnzCAU(value);
							break;

						case 13: //arithmetic right shift
							if(RS2field(incode)==23)	//+n
								value = regS(RS1field(incode))>>1;
							else
								value = regS(RS1field(incode))>>reg(RS2field(incode));
							reg(RBfield(incode)) = value&0xFFFF;
							SetCCnzCAU(value);
							break;

						case 2: // subtract N-rD
							value = regS(RS1field(incode));
							val64 = (int64_t)value - (int64_t)regS(RS2field(incode));
							reg(RBfield(incode)) = SetCCCAU16(val64, value);
							break;

						case 4: // subtract rD-N
							value = regS(RS1field(incode));
							val64 = (int64_t)value - (int64_t)regS(RS2field(incode));
							reg(RBfield(incode)) = SetCCCAU16(val64, value);
							break;

						case 3: // reverse carry add
							value = (int16_t)bitreverse(regS(RS1field(incode)), 16);
							value += (int16_t)bitreverse(regS(RS2field(incode)), 16);
							value = (int16_t)bitreverse(value, 16);
							reg(RBfield(incode)) = value;
							SetCCnzCAU(value);
							break;

						case 6: //and with complement
							value = regS(RS1field(incode))&~regS(RS2field(incode));
							reg(RBfield(incode)) = value;
							SetCCnzCAU(value);
							break;

						case 9:	//rotate right through carry
							tempc = CCcf<<12; //shift c (bit 3) to bit 15
							if(reg(RS1field(incode))&1) {
								value = (int32_t)((((uint16_t)regS(RS1field(incode)))>>1)|tempc);
								CC|=CCc;	//set c
							} else {
								value = (int32_t)((((uint16_t)regS(RS1field(incode)))>>1)|tempc);
								CC&=~CCc;	//clear c
							}
							if(!value) CC|=CCz; else CC&=~CCz;	//set/clear z
							if(value<0) CC|=CCn; else CC&=~CCn; //set/clear n
							CC&=~CCv;		//clear v
							break;

						case 11: //rotate left
							tempc = CCcf>>3; //shift c to bit 0
							value = regS(RS1field(incode));
							val64 = (int64_t)value << 1;
							value = SetCCCAU16(val64,value);
							value |= tempc;
							break;

						case 8: //exclusive or
							value = regS(RS1field(incode))^regS(RS2field(incode));
							reg(RBfield(incode)) = value;
							SetCCnzCAU(value);
							break;

						case 10: //or
							value = regS(RS1field(incode))|regS(RS2field(incode));
							reg(RBfield(incode)) = value;
							SetCCnzCAU(value);
							break;

						case 14: //and
							value = regS(RS1field(incode))&regS(RS2field(incode));
							reg(RBfield(incode)) = value;
							SetCCnzCAU(value);
							break;

						default:	//Fcode 5 is reserved
							DSP_undefined();
							break;
						}
					}
				}
				break;

			case 38:
				//32-bit ALU instruction, format 6b,6d
				if((incode&0x02000000)>>25)	//indicates immediate
				{
					switch(FCAUfield(incode))
					{
					case 7:	//compare
						value = reg(RBfield(incode));
						val64 = (int64_t)value - (int64_t)Nfield(incode);
						value = SetCCCAU(val64,value);
						break;

					case 15: //and, no store
						value = reg(RBfield(incode))&Nfield(incode);
						SetCCnzCAU(value);
						break;

					case 1: //left shift
						value = reg(RBfield(incode)) << Nfield(incode);
						reg(RBfield(incode)) = value;
						SetCCnzCAU(value);
						break;

					case 12: //logical right shift
						value = (int32_t)(((uint32_t)reg(RBfield(incode))) >> Nfield(incode));
						reg(RBfield(incode)) = value;
						SetCCnzCAU(value);
						break;

					case 13: //arithmetic right shift
						value = reg(RBfield(incode)) >> Nfield(incode);
						reg(RBfield(incode)) = value;
						SetCCnzCAU(value);
						break;

					case 2: // subtract N-rD
						value = Nfield(incode);
						val64 = (int64_t)value - (int64_t)reg(RBfield(incode));
						reg(RBfield(incode)) = SetCCCAU(val64,value);
						break;

					case 4: // subtract rD-N
						value = reg(RBfield(incode));
						val64 = (int64_t)value - (int64_t)Nfield(incode);
						reg(RBfield(incode)) = SetCCCAU(val64,value);
						break;

					case 3: // reverse carry add
						//not supported
						value = bitreverse(regS(RBfield(incode)), 32);
						value += bitreverse((int32_t)Nfield(incode), 32);
						value = bitreverse(value, 32);
						reg(RBfield(incode)) = value;
						SetCCnzCAU(value);
						break;

					case 6: //and with complement
					case 9:	//rotate right
					case 11: //rotate left
						//not with immediate operand
						DSP_undefined();
						break;

					case 8: //exclusive or
						value = reg(RBfield(incode))^Nfield(incode);
						reg(RBfield(incode)) = value;
						SetCCnzCAU(value);
						break;

					case 10: //or
						value = reg(RBfield(incode))|Nfield(incode);
						reg(RBfield(incode)) = value;
						SetCCnzCAU(value);
						break;

					case 14: //and
						value = reg(RBfield(incode))&Nfield(incode);
						reg(RBfield(incode)) = value;
						SetCCnzCAU(value);
						break;

					default:	//Fcode 5 is reserved
						DSP_undefined();
						break;
					}
				} else {
					if(TestCfield(Cfield(incode)))	//ie not always false
					{
						switch(FCAUfield(incode))
						{
						case 7:	//compare
							value = reg(RS1field(incode));
							val64 = (int64_t)value - (int64_t)reg(RS2field(incode));
							value = SetCCCAU(val64, value);
							break;

						case 15: //and, no store
							value = reg(RS1field(incode))&reg(RS2field(incode));
							SetCCnzCAU(value);
							break;

						case 0: //add
							value = reg(RS1field(incode));
							val64 = (int64_t)value + (int64_t)reg(RS2field(incode));
							reg(RBfield(incode)) = SetCCCAU(val64, value);
							break;
						
						case 1: //left shift
							if(RS2field(incode)==23)	//+n
								value = reg(RS1field(incode))<<1;
							else
								value = reg(RS1field(incode))<<reg(RS2field(incode));
							reg(RBfield(incode)) = value;
							SetCCnzCAU(value);
							break;

						case 12: //logical right shift
							if(RS2field(incode)==23)	//+n
								value = (int32_t)(((uint32_t)reg(RS1field(incode)))>>1);
							else
								value = (int32_t)(((uint32_t)reg(RS1field(incode)))>>reg(RS2field(incode)));
							reg(RBfield(incode)) = value;
							SetCCnzCAU(value);
							break;

						case 13: //arithmetic right shift
							if(RS2field(incode)==23)	//+n
								value = reg(RS1field(incode))>>1;
							else
								value = reg(RS1field(incode))>>reg(RS2field(incode));
							reg(RBfield(incode)) = value;
							SetCCnzCAU(value);
							break;

						case 2: // subtract N-rD
							value = reg(RS1field(incode));
							val64 = (int64_t)value - (int64_t)reg(RS2field(incode));
							reg(RBfield(incode)) = SetCCCAU(val64, value);
							break;

						case 4: // subtract rD-N
							value = reg(RS1field(incode));
							val64 = (int64_t)value - (int64_t)reg(RS2field(incode));
							reg(RBfield(incode)) = SetCCCAU(val64, value);
							break;

						case 3: // reverse carry add
							value = bitreverse(regS(RS1field(incode)), 32);
							value += bitreverse(regS(RS2field(incode)), 32);
							value = bitreverse(value, 32);
							reg(RBfield(incode)) = value;
							SetCCnzCAU(value);
							break;

						case 6: //and with complement
							value = reg(RS1field(incode))&~reg(RS2field(incode));
							reg(RBfield(incode)) = value;
							SetCCnzCAU(value);
							break;

						case 9:	//rotate right through carry
							tempc = CCcf<<28; //shift c (bit 3) to bit 31
							if(reg(RS1field(incode))&1) {
								value = (int32_t)(((uint32_t)reg(RS1field(incode)))>>1);
								CC|=CCc;	//set c
							} else {
								value = (int32_t)(((uint32_t)reg(RS1field(incode)))>>1);
								CC&=~CCc;	//clear c
							}
							value |= tempc;	//old c becomes bit 31
							if(!value) CC|=CCz; else CC&=~CCz;	//set/clear z
							if(value<0) CC|=CCn; else CC&=~CCn; //set/clear n
							CC&=~CCv;		//clear v
							break;

						case 11: //rotate left
							tempc = CCcf>>3; //shift c to bit 0
							value = reg(RS1field(incode));
							val64 = ((int64_t)value) << 1;
							value = SetCCCAU(val64,value);
							value |= tempc;
							break;

						case 8: //exclusive or
							value = reg(RS1field(incode))^reg(RS2field(incode));
							reg(RBfield(incode)) = value;
							SetCCnzCAU(value);
							break;

						case 10: //or
							value = reg(RS1field(incode))|reg(RS2field(incode));
							reg(RBfield(incode)) = value;
							SetCCnzCAU(value);
							break;

						case 14: //and
							value = reg(RS1field(incode))&reg(RS2field(incode));
							reg(RBfield(incode)) = value;
							SetCCnzCAU(value);
							break;

						default:	//Fcode 5 is reserved
							DSP_undefined();
							break;
						}
					}
				}
				break;

			case 7:
				//move DA instruction LOAD/STORE rD=*L, *L=rS
				//need to set upper 16 bits of address if necessary to 0x5003 based on pcw[10]
				//disables interrupts for one cycle
				if(io_pcw & 0x0400)	//pcw bit 10
					baseaddr = map1_ROM_start;
				else
					baseaddr = map0_ROM_start;

				if((incode&0x01000000)>>24)	//From a reg
				{
					switch(Wfield(incode))
					{
					case 0:	//char
					case 1:
						DSP_set_char(baseaddr | RLfield(incode), DSP_swapl(reg(RHfield(incode))&0xFF));
						break;

					case 2: //short
					case 3:
						DSP_set_short(baseaddr | RLfield(incode), DSP_swapw(reg(RHfield(incode))&0xFFFF));
						break;

					case 4: //unsigned high char
						DSP_set_char(baseaddr | RLfield(incode), DSP_swapl((reg(RHfield(incode))&0xFF00)>>8)<<8);
						break;

					case 7:	//long
						DSP_set_long(baseaddr | RLfield(incode), DSP_swapl(reg(RHfield(incode))));
						break;

					default:
						DSP_undefined();
						break;
					}
							
				} else {	//To a reg
					switch(Wfield(incode))
					{
					case 0:	//unsigned char
						reg(RHfield(incode)) = DSP_swapl((uint32_t)DSP_get_char(baseaddr | RLfield(incode)));
						break;

					case 1: //char
						reg(RHfield(incode)) = DSP_swapl((int32_t)DSP_get_char(baseaddr | RLfield(incode)));
						break;

					case 2: //unsigned short
						reg(RHfield(incode)) = DSP_swapl((uint32_t)DSP_get_short(baseaddr | RLfield(incode)));
						break;

					case 3: //short
						reg(RHfield(incode)) = DSP_swapl((int32_t)DSP_get_short(baseaddr | RLfield(incode)));
						break;

					case 4: //unsigned high char
						reg(RHfield(incode)) = DSP_swapl(((uint32_t)DSP_get_short(baseaddr | RLfield(incode)))<<8);
						break;

					case 7:	//long
						reg(RHfield(incode)) = DSP_swapl(DSP_get_long(baseaddr | RLfield(incode)));
						break;

					default:
						DSP_undefined();
						break;
					}
				}
				break;

			case 39:
				//move IO/post inc/mem instruction
				switch((incode&0x400)>>10)
				{
				case 0:	//move post inc or mem i/o
					switch((incode&0x02000000)>>25)
					{
					case 0:  //LOAD/STORE rD=MEM, MEM=rS ie move post inc 7c
						if((incode&0x01000000)>>24)	//move from reg to mem
						{
							switch(Wfield(incode))
							{
							case 0:	//char
							case 1:
								//*((char *)amiga_mem_base+reg(RPfield(incode))) = swapl(reg(RHfield(incode))&0xFF);
								DSP_set_char(reg(RPfield(incode)), DSP_swapl(reg(RHfield(incode))&0xFF));
								pinc=1;
								break;

							case 2: //short
							case 3:
								//*((int16_t*)amiga_mem_base+((reg(RPfield(incode)))>>1)) = (swapw(reg(RHfield(incode))&0xFFFF));
								DSP_set_short(reg(RPfield(incode)), DSP_swapw(reg(RHfield(incode))&0xFFFF));
								pinc=2;
								break;

							case 4: //unsigned high char
								//*((char *)amiga_mem_base+reg(RPfield(incode))) = swapl((reg(RHfield(incode))&0xFF00)>>8)<<8;
								DSP_set_char(reg(RPfield(incode)), DSP_swapl((reg(RHfield(incode))&0xFF00)>>8)<<8);
								pinc=1;
								break;

							case 7:	//long
								//*(amiga_mem_base+((reg(RPfield(incode)))>>2)) = swapl(reg(RHfield(incode)));
								DSP_set_long(reg(RPfield(incode)), DSP_swapl(reg(RHfield(incode))));
								pinc=4;
								break;

							default:
								DSP_undefined();
								break;
							}
							//do post inc
							switch(RIfield(incode))
							{
							case 0:		//no post inc
								break;

							case 22:	//post dec
								reg(RPfield(incode))-=pinc;
								break;

							case 23:	//post inc
								reg(RPfield(incode))+=pinc;
								break;

							default:	//RI
								reg(RPfield(incode))+=reg(RIfield(incode));
								break;
							}
						} else	{	//move to reg from mem
							switch(Wfield(incode))
							{
							case 0:	//unsigned char
								//reg(RHfield(incode)) = swapl((uint32_t)(*((unsigned char *)amiga_mem_base+reg(RPfield(incode)))));
								reg(RHfield(incode)) = DSP_swapl((uint32_t)DSP_get_char(reg(RPfield(incode))));
								pinc=1;
								break;

							case 1: //char
								//reg(RHfield(incode)) = swapl((int32_t)(*((char *)amiga_mem_base+reg(RPfield(incode)))));
								reg(RHfield(incode)) = DSP_swapl((int32_t)DSP_get_char(reg(RPfield(incode))));
								pinc=1;
								break;

							case 2: //unsigned short
								//reg(RHfield(incode)) = swapl((uint32_t)(*((uint16_t *)amiga_mem_base+((reg(RPfield(incode)))>>1))));
								reg(RHfield(incode)) = DSP_swapl((uint32_t)DSP_get_short(reg(RPfield(incode))));
								pinc=2;
								break;

							case 3: //short
								//reg(RHfield(incode)) = swapl((int32_t)(*((int16_t *)amiga_mem_base+((reg(RPfield(incode)))>>1))));
								reg(RHfield(incode)) = DSP_swapl((int32_t)DSP_get_short(reg(RPfield(incode))));
								pinc=2;
								break;

							case 4: //unsigned high char
								//reg(RHfield(incode)) = swapl(((uint32_t)(*((unsigned char *)amiga_mem_base+reg(RPfield(incode)))))<<8);
								reg(RHfield(incode)) = DSP_swapl(((uint32_t)DSP_get_short(reg(RPfield(incode))))<<8);
								pinc=1;
								break;

							case 7:	//long
								//reg(RHfield(incode)) = swapl(*(amiga_mem_base+((reg(RPfield(incode)))>>2)));
								reg(RHfield(incode)) = DSP_swapl(DSP_get_long(reg(RPfield(incode))));
								pinc=4;
								break;

							default:
								DSP_undefined();
								break;
							}
							//do post inc
							switch(RIfield(incode))
							{
							case 0:		//no post inc
								break;

							case 22:	//post dec
								reg(RPfield(incode))-=pinc;
								break;

							case 23:	//post inc
								reg(RPfield(incode))+=pinc;
								break;

							default:	//RI
								reg(RPfield(incode))+=reg(RIfield(incode));
								break;
							}
						}
						break; //end 7c

					case 1:	//move io 7d instruction
						switch(RHfield(incode))
						{
						case 0:		//ps
						case 8:		//emr
						case 12:	//pcw
						case 14:	//dauc
						case 15:	//ctr
							break;

						default:	//only the above io registers are supported by the DSP3210
							DSP_undefined();
							break;
						}

						if((incode&0x01000000)>>24)	//move from reg to mem
						{
							switch(Wfield(incode))
							{
							case 0:	//char
								DSP_set_char(reg(RPfield(incode)), DSP_swapl(io(RHfield(incode))&0xFF));
								pinc=1;
								break;

							case 2: //short
								DSP_set_short(reg(RPfield(incode)), DSP_swapw(io(RHfield(incode))&0xFFFF));
								pinc=2;
								break;

							case 7:	//long
								DSP_set_long(reg(RPfield(incode)), DSP_swapl(io(RHfield(incode))));
								pinc=4;
								break;

							default:
								DSP_undefined();
								break;
							}
							//do post inc
							switch(RIfield(incode))
							{
							case 0:		//no post inc
								break;

							case 22:	//post dec
								reg(RPfield(incode))-=pinc;
								break;

							case 23:	//post inc
								reg(RPfield(incode))+=pinc;
								break;

							default:	//RI
								reg(RPfield(incode))+=reg(RIfield(incode));
								break;
							}
						} else	{	//move to reg from mem
							if(RHfield(incode)!=15)	//cannot write to ctr
							switch(Wfield(incode))
							{
							case 0:	//unsigned char - do not change other bits
								io(RHfield(incode)) = (io(RHfield(incode))&0xFFFFFF00) | (DSP_swapl((uint32_t)DSP_get_char(reg(RPfield(incode)))) & 0xFF);
								pinc=1;
								break;

							case 2: //unsigned short - do not change high word
								io(RHfield(incode)) = (io(RHfield(incode))&0xFFFF0000) | (DSP_swapl((uint32_t)DSP_get_short(reg(RPfield(incode)))) & 0xFFFF);
								pinc=2;
								break;

							case 7:	//long
								io(RHfield(incode)) = DSP_swapl(DSP_get_long(reg(RPfield(incode))));
								pinc=4;
								break;

							default:
								DSP_undefined();
								break;
							}
							//do post inc
							switch(RIfield(incode))
							{
							case 0:		//no post inc
								break;

							case 22:	//post dec
								reg(RPfield(incode))-=pinc;
								break;

							case 23:	//post inc
								reg(RPfield(incode))+=pinc;
								break;

							default:	//RI
								reg(RPfield(incode))+=reg(RIfield(incode));
								break;
							}
						}
						break; //end 7d
					}
					break; //end 7c and 7d

				case 1: //move io 7b instruction
					switch(incode)
					{
                    case static_cast<int32_t>(0x09DE0040A):
						//execute waiti
						DSP_reg_irsh = DSP_swapl(DSP_get_long(reg_PC + 4));
						DSP_irsh_flag = 4;	//execute one more instruction, then pause DSP
						DSP_irsh_addr = reg_PC + 4;
						DSP_irsh_pcw = DSP_io_r[12];
						break;

                    case static_cast<int32_t>(0x9D00040A):
						//execute sftrst = reset
						DSP_reset();
						break;

                    case static_cast<int32_t>(0x9D60040A):
						//execute breakpoint
						DSP_breakpoint();
						break;

					default:
						switch(RIfield(incode))
						{
						case 0:		//ps 16
						case 8:		//emr 16
						case 10:	//spc 32
						case 12:	//pcw 16
						case 14:	//dauc 8
						case 15:	//ctr 8
							break;

						default:	//only the above io registers are supported by the DSP3210
							DSP_undefined();
							break;
						}
						if((incode&0x01000000)>>24)	//move from reg to io, iorD = rS
						{
							if(RHfield(incode)!=15)	//cannot write to ctr
							{
								switch(Wfield(incode))
								{
								case 1:	//char
									io(RIfield(incode)) = (io(RIfield(incode)) & 0xFFFFFF00) | (reg(RHfield(incode)) & 0xFF);
									break;

								case 3: //short
									if(RIfield(incode)<14)
										io(RIfield(incode)) = (io(RIfield(incode)) & 0xFFFF0000) | (reg(RHfield(incode)) & 0xFFFF);
									else
										//cannot read 16 bits from an 8 bit io reg
										DSP_undefined();
									break;

								case 4: //unsigned high char
									if(RIfield(incode)<14)
										io(RIfield(incode)) = (io(RIfield(incode)) & 0xFFFF00FF) | (reg(RHfield(incode)) & 0xFF00);
									else
										//cannot read 16 bits from an 8 bit io reg
										DSP_undefined();
									break;

								case 7:
									if(RIfield(incode) == 10)
										io(RIfield(incode)) = reg(RHfield(incode));
									else
										DSP_undefined();
									break;

								default:
									DSP_undefined();
									break;
								}
							} else {
								//exception due to write to ctr attempt?
							}
						} else	{	//move to reg from io rD = iorS
							switch(Wfield(incode))
							{
							case 0:	//unsigned char
								reg(RHfield(incode)) = io(RIfield(incode)) & 0xFF;
								break;

							case 1: //signed char
								reg(RHfield(incode)) = (int32_t)((char)(io(RIfield(incode)) & 0xFF));
								break;

							case 2: //unsigned short
								if(RIfield(incode)<14)
									reg(RHfield(incode)) = io(RIfield(incode)) & 0xFFFF;
								else
									DSP_undefined();
								break;

							case 3: //signed short
								if(RIfield(incode)<14)
									reg(RHfield(incode)) = (int32_t)((int16_t)(io(RIfield(incode)) & 0xFFFF));
								else
									DSP_undefined();
								break;

							case 4: //high byte
								reg(RHfield(incode)) = (io(RIfield(incode)) & 0xFF) << 8;
								break;

							case 7: //long
								if(RIfield(incode)==10)	//read from spc to reg
									reg(RHfield(incode)) = io(RIfield(incode));
								else
									DSP_undefined();
								break;

							default:
								DSP_undefined();
								break;
							}
						}
						break;	// end test for special instructions
					}
					break;	//end 7b
				}
				break; //end case 39

			default:
				DSP_undefined();
				break;
			}
			break;
		}
		break;
	}	
	return ;
}

/********************************************************/
/* Functions to support doinst                          */
/********************************************************/

// Extract the two parts of an X, Y or Z field (depending on field)
// into the return value (fld1) and fld2
int32_t DAUXYZ(int32_t incode, int32_t field, int32_t *fld2)
{
	int32_t fld1,temp;
	switch(field)
	{
	case 1:
		fld1=Xfield(incode)>>3;
		temp=Xfield(incode)&0x7;
		break;
	case 2:
		fld1=Yfield(incode)>>3;
		temp=Yfield(incode)&0x7;
		break;
	case 3:
		fld1=Zfield(incode)>>3;
		temp=Zfield(incode)&0x7;
		break;
	}
	
	if(fld1)
	{
		if(temp>0&&temp<6)
			*fld2=temp+14;
		else
			*fld2=temp;
	} else {
		*fld2=temp;
	}
	return fld1;
}

//return the X or Y field determined by the field parameters
// as a _BE_ value
int32_t GetXYfield(int32_t fld1,int32_t fld2,int32_t incode)
{
	int32_t temp,g;
	int32_t postinc=4;
	intfloat conv;

	if((incode&0xF8000000)>>27==15)	//true if DAU5
	{		//change size field to short on int16/float16
		g=Gfield(incode);
		switch(g)
		{
		case 2:
		case 3:
			postinc=2;
			break;

		default:
			break;
		}
	}

	if(!fld1)
	{
		if(fld2>3)
		{
			DSP_undefined();
			return 0;
		}
		//=aN
		conv.f = DSP_reg_a[fld2];
		temp = DSP_swapl(conv.i);
	} else {
		switch(fld2)
		{
		case 0:
			//=*rP
			temp = DSP_get_long(DSP_reg_r[fld1]);
			break;

		case 6:
			//=*rP--
			temp = DSP_get_long(DSP_reg_r[fld1]);
			DSP_reg_r[fld1]-=postinc;
			break;

		case 7:
			//=*rP++
			temp = DSP_get_long(DSP_reg_r[fld1]);
			DSP_reg_r[fld1]+=postinc;
			break;

		default:
			//=*rP++rI
			temp = DSP_get_long(DSP_reg_r[fld1]);
			DSP_reg_r[fld1]+=DSP_reg_r[fld2];
			break;
		}

		if((incode>>27)!=15) 
			//Not DAU5
			//having read a BE DSP float (as an int) from mem, convert to
			//internal BE IEEE float (as an int)
			return(DSP_swapl(DSP_dsp2ieee(DSP_swapl(temp))));	

		switch(Gfield(incode))
		{
		case 2:		//do nothing on float 16/32, dsp
		case 8:
		case 13:
			break;

		default:
			//having read a BE DSP float (as an int) from mem, convert to
			//internal BE IEEE float (as an int)
			temp = DSP_swapl(DSP_dsp2ieee(DSP_swapl(temp)));	
			break;
		}
	}
	return temp;
}

// Set up jump instructions with 3 clock latencies
void DoLat3(int32_t pcval)
{
	int32_t lat3ptr=0;
	if(!latency3[0][0]) lat3ptr=1;	//first slot occupied
	latency3[lat3ptr][1]=pcval;		//new PC value
	latency3[lat3ptr][0]=2;			//2 ticks to execute
	return;
}

// Set up Z-field writes with 2 instruction latency
// and post inc/dec
void SetZfield(int32_t fld1, int32_t fld2, int32_t temp)	
//temp must be converted from BE IEEE to DSP BE
{
	int32_t postinc=4;		//postinc always 4
	intfloat conv;

	if(fld1)					//need to set Zfield
	{							//deal with write latency 1 here
		int32_t temp2 = DSP_swapl(temp); //now LE
		//float f = *((float *)&(temp2)); //input -> float (still LE IEEE) '''
		conv.i = temp2;
		float f = conv.f;
		temp2 = DSP_swapl(DSP_float(f));
		//DSP_history_mem[0][0] = (int32_t)(amiga_mem_base+(DSP_reg_r[fld1]>>2));
		DSP_history_mem[0][0] = DSP_reg_r[fld1];	//>>2;
		DSP_history_mem[1][0] = temp2;
		DSP_history_mem[2][0] = 1;	//delayed write enabled

		switch(fld2)
		{
		case 0:
			break;

		case 6: //*rP--=
			DSP_reg_r[fld1]-=postinc;
			break;

		case 7:	//*rP++=
			DSP_reg_r[fld1]+=postinc;
			break;

		default:	//*rP++rI=
			DSP_reg_r[fld1]+=DSP_reg_r[fld2];
			break;
		}
	}
}

// Execute DAU5 type instruction
void doDAU5(int32_t incode)
{
	int32_t value, fld1, fld2;
	float ftemp;
	fld1=DAUXYZ(incode,2,&fld2);	//get Yfield values
	value=GetXYfield(fld1,fld2,incode);		//read value from Yfield reg and post inc
									//NB value is BE 
	int32_t m0,m1, m2, m3;		//###
	intfloat conv;

	switch(Gfield(incode))
	{
	case 0:	//ic
	case 1:	//oc
		//Unsupport instruction
		break;

	case 2:
		//float16
		ftemp = (float)DSP_swapl((value>>16));	//want first short
		conv.f = ftemp;
		value = DSP_swapl(conv.i);
		break;

	case 3:
		//int16
		m0 = DSP_swapl(value);			//BE to LE
		//get float value
		conv.i = m0;
		ftemp = conv.f;
		ftemp = round_dsp(ftemp);		//round
		value = (int32_t)ftemp;	//convert to int
		if(value>0x7FFF||value<-0x8000)
			value=0x7FFF;
		else
			value&=0xFFFF;
		value = DSP_swapl(value);		//convert to BE
		break;

	case 4:
		//round
		//nothing to do as it rounds 40 bits to 32 bit
		break;

	case 5:
		//ifalt ie do nothing if Z-flag set or N-flag not set
		if(!(CC&CCN))
			return;	 //Do nothing if Z-flag not set
		//otherwise no other manipulation needed		
		break;
	
	case 6:
		//ifaeq
		if(!(CC&CCZ))
			return;	 //Do nothing if Z-flag not set
		//otherwise no other manipulation needed		
		break;

	case 7:
		//ifagt ie do nothing if Z-flag set or N-flag not set
		if(CC&CCZ||CC&CCN) 
			return;	 //Do nothing if Z-flag not set
		//otherwise no other manipulation needed		
		break;

	case 8:
		//float32
		value = DSP_swapl(value);
		ftemp = (float)value;
		conv.f = ftemp;
		value = DSP_swapl(conv.i);
		break;

	case 9:
		//int32
		m0 = DSP_swapl(value);			//BE to LE IEEE
		//get float value
		conv.i = m0;
		ftemp = conv.f;
		ftemp = round_dsp(ftemp);		//round
		value = DSP_swapl((int32_t)ftemp);	//convert to int and BE
		break;

	case 10:
	case 11:
		DSP_undefined();
		break;

	case 12:
		//ieee
		//value = swapl(DSP_dsp2ieee(swapl(value)));
		m0 = DSP_swapl(value);
		if(((m0 & 0x7F800000) == 0x7F800000) && ((m0 & 0x007FFFFF) != 0))
			DSP_exception_NaN();
		break;

	case 13:
		//dsp
		/*m0 = swapl(value);					//get BE IEEE float into LE IEEE
		ftemp = DSP_float(*(float *)&m0);	//convert to DSP float format
		value = swapl(*(int *)&ftemp);		//put into a BE int*/
		break;

	case 14:
		//seed
		m0 = DSP_swapl(value);			//value now LE IEEE
		//IEEE LE -> DSP LE
		conv.i = m0;
		m3 = DSP_float(conv.f);
		//m1 = ((~DSP_float(*(float *)&m0))^0x80000000); //invert all bits except sign
		m1 = ((~m3) ^ 0x80000000);
		m2 = DSP_dsp2ieee(m1);			//DSP LE -> IEEE LE
		
		value = DSP_swapl(m2);			//value is IEEE BE
		break;

	default:
		//reserved
		DSP_undefined();
		break;
	}

	if(Gfield(incode)==3 || Gfield(incode)==9)		//int16 or int32
		DSP_reg_a[NDAUfield(incode)]=(float)DSP_swapl(value);	//set aN to int value
	else {
		//set aN to value
		conv.i = DSP_swapl(value);
		DSP_reg_a[NDAUfield(incode)] = conv.f;
	}

	switch (Gfield(incode))	//set flags on required instructions
	{
	case 0:	//ic
	case 2:	//float16
	case 4:	//round
	case 8:	//float32
	case 14:	//seed
		if(DSP_swapl(value)<0) CC|=CCN; else CC&=~CCN;
		if(!DSP_swapl(value))  CC|=CCZ; else CC&=~CCZ;
		CC&=~CCU;
		CC&=~CCV;
		break;

	case 13:	//dsp U and V already set
		if(DSP_swapl(value)<0) CC|=CCN; else CC&=~CCN;	
		if(!DSP_swapl(value))  CC|=CCZ; else CC&=~CCZ;
		break;

	default:
		break;
	}

	fld1=DAUXYZ(incode,3,&fld2);	//get Zfield values
	switch(Gfield(incode))
	{
	case 3:
		//int16 exception here.  We need to do a version of
		//SetZfield but without the conversion to a DSP float 
		
		if(fld1)					//need to set Zfield
		{							//deal with write latency 1 here
			DSP_history_mem[0][0] = DSP_reg_r[fld1];
			DSP_history_mem[1][0] = value;
			DSP_history_mem[2][0] = 2;	//delayed write enabled

			switch(fld2)
			{
			case 0:
				//*rP=
				break;

			case 6:
				//*rP--=
				DSP_reg_r[fld1]-=2;
				break;

			case 7:
				//*rP++=
				DSP_reg_r[fld1]+=2;
				break;

			default:
				//*rP++rI=
				DSP_reg_r[fld1]+=DSP_reg_r[fld2];
				break;
			}
		}
		break;
	case 9:
	case 12:
		//int32, ieee exception here.  We need to do a version of
		//SetZfield but without the conversion to a DSP float 
		
		if(fld1)					//need to set Zfield
		{							//deal with write latency 1 here
			//DSP_history_mem[0][0] = (int32_t)(amiga_mem_base+(DSP_reg_r[fld1]>>2));
			DSP_history_mem[0][0] = DSP_reg_r[fld1];
			DSP_history_mem[1][0] = value;
			DSP_history_mem[2][0] = 1;	//delayed write enabled

			switch(fld2)
			{
			case 0:
				//*rP=
				break;

			case 6:
				//*rP--=
				DSP_reg_r[fld1]-=4;
				break;

			case 7:
				//*rP++=
				DSP_reg_r[fld1]+=4;
				break;

			default:
				//*rP++rI=
				DSP_reg_r[fld1]+=DSP_reg_r[fld2];
				break;
			}
		}
		break;

	default:
		SetZfield(fld1, fld2, value);	//and set Zfield if needed
		break;
	} 
}

// Extract float value out of M field
float Mval(int32_t incode)
{
	int32_t m = Mfield(incode);
	switch(m)
	{
	case 4:
		return 0.0;
		break;

	case 5:
		return 1.0;
		break;

	case 0:
	case 1:
	case 2:
	case 3:
		return DSP_reg_a[m];

	default:
		return 0;
	}
}

// Set condition codes for DAU instructions
float SetCC(double dtemp)
{
	if(dtemp<0)		CC|=CCN; else CC&=~CCN;
	if(dtemp==0)	CC|=CCZ; else CC&=~CCZ;

	//test for under/overflow situations
	if(dtemp>max_pos_float || dtemp<max_neg_float) DSP_exception_UOflow();
	if((dtemp>0 && dtemp<min_pos_float) || (dtemp<0 && dtemp>min_neg_float)) DSP_exception_UOflow();

	//test for UV flags
	if(dtemp>3.40282e28||dtemp<-3.40282e28) { CC|=CCV; dtemp=0;} else CC&=~CCV;
	if(dtemp<5.87747e-39&&dtemp>-5.87747e-39&&dtemp!=0) { CC|=CCU; dtemp=0;} else CC&=~CCU;
	
	return((float)dtemp);
}

//Set confition codes for 32 bit CAU instructions
int32_t SetCCCAU(int64_t res, int32_t input)
{
	int64_t in = (int64_t)input;
	if(res<0)	CC|=CCn; else CC&=~CCn;
	if(res==0)	CC|=CCz; else CC&=~CCz;
	if((res&hi32)^(in&hi32)) CC|=CCc; else CC&=~CCc;  //set c if hi-32 bits change
	if(((res&bt32)^(in&bt32))&!CCcf) CC|=CCv; else CC&=~CCv; //set v if bit 32 changes and c unset
	return((int32_t)res);
}

//Set condition codes for 16 bit CAU instructions
int32_t SetCCCAU16(int64_t res, int32_t input)
{
	int64_t in = (int64_t)input;
	if(res<0)	CC|=CCn; else CC&=~CCn;
	if(res==0)	CC|=CCz; else CC&=~CCz;
	if((res&hi48)^(in&hi48)) CC|=CCc; else CC&=~CCc;  //set c if hi-48 bits change
	if(((res&bt16)^(in&bt16))&!CCcf) CC|=CCv; else CC&=~CCv; //set v if bit 16 changes and c unset
	return((int32_t)res);
}

//Set n,z condition codes, clear c, v
void SetCCnzCAU(int32_t res)
{
	if(res<0)	CC|=CCn; else CC&=~CCn;
	if(res==0)	CC|=CCz; else CC&=~CCz;
	CC&=~CCc; //clear c
	CC&=~CCv; //clear v
	return;
}

//Test C field
int32_t TestCfield(int32_t code)	//test CCs but U and V not supported
{
	int32_t res=0;
	switch(code)
	{
	case 0:  //false
		break;

	case 1:	//true
		res=1;
		break;

	case 2:	//pl
		if(!CCnf) res=1;
		break;

	case 3:	//mi
		if(CCnf) res=1;
		break;

	case 4:	//ne
		if(!CCzf) res=1;
		break;

	case 5:	//eq
		if(CCzf) res=1;
		break;

	case 6: //vc
		if(!CCvf) res=1;
		break;

	case 7: //vs
		if(CCvf) res=1;
		break;	

	case 8: //cc
		if(!CCcf) res=1;
		break;

	case 9: //cs
		if(CCcf) res=1;
		break;

	case 10: //ge
		if(!(CCnf^CCvf)) res=1;
		break;

	case 11: //lt
		if(CCnf^CCvf) res=1;
		break;

	case 12: //gt
		if(!CCzf&!(CCnf^CCvf)) res=1;
		break;

	case 13: //le
		if(CCzf|(CCnf^CCvf)) res=1;
		break;

	case 14: //hi
		if(!CCcf&!CCzf) res=1;
		break;

	case 15: //ls
		if(CCcf|CCzf) res=1;
		break;

	case 18: //age
		if(!(DSP_history_CC[3]&CCN)) res=1;
		break;

	case 19: //alt
		if(DSP_history_CC[3]&CCN) res=1;
		break;

	case 20: //ane
		if(!(DSP_history_CC[3]&CCZ)) res=1;
		break;

	case 21: //aeq
		if(DSP_history_CC[3]&CCZ) res=1;
		break;

	case 24: //agt
		if(!(DSP_history_CC[3]&CCN)&!(DSP_history_CC[3]&CCZ)) res=1;
		break;

	case 25: //ale
		if((DSP_history_CC[3]&CCN)|(DSP_history_CC[3]&CCZ)) res=1;
		break;

	}
	return res;
}



/********************************************************/
/* Utility functions                                    */
/********************************************************/

//BE <-> LE craziness
int32_t DSP_swapl(int32_t a)
{
	return ((a & 0x000000FF) << 24) |
      ((a & 0x0000FF00) <<  8) |
      ((a & 0x00FF0000) >>  8) |
      ((a & 0xFF000000) >> 24);
}

int16_t DSP_swapw(int16_t a)
{
	return ((a & 0x00FF) << 8) | ((a & 0xFF00) >> 8);
}

//map register code onto a uniform register number
int32_t DSP_regno(int32_t r)
{
	if(r<15)
		return r;
	if(r==15)
		return 23;		//map PC to reg 23
	if(r>16&&r<22)
		return r-2;		//map regs 15-19
	if(r>23&&r<27)
		return r-4;
	if(r==30)
		return 24;		//map pcsh to reg 24
	return 25;			//trap illegal
}

// LE IEEE to LE DSP
int32_t DSP_float(float f)
{
	int32_t neg = 0;
	int32_t mant, exp;
	intfloat intf;

	if(f < 0.0) {
		f = -f;
		neg = 1;
	}
	intf.f = f;

	mant = intf.i & 0x007FFFFF;
	exp = (intf.i >> 23) & 0x000000FF;

	if(exp != 0)
	{
		exp += 1;
		CC&=~CCU;	//clear underflow flag
	}
	else
	{
		mant = 0; // unnormalized underflows ==> 0.0
		CC|=CCU;	//set underflow flag
	}
	if(exp > 0xFF) {
		exp = 0xFF;
		mant = 0x007FFFFFL;
		CC|=CCV;	//set overflow flag
	} else CC&=~CCV;	//clear overflow flag
	mant = (mant << 8) | exp;

	return neg ? DSP_negfloat(mant) : mant;
}

int32_t DSP_negfloat(int32_t f)
{
	int32_t mant, exp;
	if(f == 0)
		return f;

	mant = f >> 8;
	exp = f & 0xFF;

	mant = -mant;
	if(f > 0 && mant == 0) {
		exp -= 1;
		mant = 0x00800000;
	} else if(f < 0 && mant == 0)
		exp += 1;

	return (mant<<8) | exp;
}

int32_t bitreverse(int32_t n, int32_t bits)
{
	int32_t rv = 0;
	int32_t i;
	for(i = 0; i < bits; ++i, n >>= 1) {
		rv = (rv << 1) | (n & 0x01);
	}
	return rv;
}

//convert LE DSP float as int to LE IEEE float as int
int32_t DSP_dsp2ieee(int32_t f)
{
	int32_t neg = 0;
	int32_t m,e;
	
	if(!f) return f;
	if(f&0x80000000)
		neg = 1;

	e = f&0xFFL;
	m = (f>>8)&0x7FFFFF;

	if(!e) return 0; //zero even if dirty zero

	e-=1;
	//cannot be a carry as e>0

	if(neg) {
		m = (-m) & 0x7FFFFF;
		if(!m) e -= 1;
	}

	m = m|(e<<23)|(neg?0x80000000:0);

	return m;
}

static float round_dsp(float num)
{
    float integer = ceilf(num); //smallest integer >= num (-2>-2.8)
	switch((DSP_io_r[14]&0x30)>>4)
	{
	case 0:	//round nearest
	case 2:
	default:
		if(num > 0)
			return integer - num > 0.5f ? integer - 1.0f : integer;
		return integer - num >= 0.5f ? integer - 1.0f : integer;
		break;
		
	case 1:
		return integer - 1.0f;
		break;

	case 3:
		if(num > 0)
			return integer - 1.0f;
		return integer;
		break;
	}
}

/********************************************************/
/* Illegal opcode exception                             */
/*                                                      */
/********************************************************/
void DSP_exception_illegal()
{
	//non-maskable exception
	//if(!(io_emr & 0x8000))
		//return;	//return if int1 masked out

	//save chip state: DO loop status, a0-a3, dauc
	//not implemented yet
	DSP_reg_r[20] = reg_PC;
	reg_pcsh = reg_PC;	//save PC
	reg_PC = reg_evtp + 8*2;	//Illegal opcode entry
	return;
}

/********************************************************/
/* Under/overflow exception                             */
/*                                                      */
/********************************************************/
void DSP_exception_UOflow()
{
	if(!(io_emr & 0x0020))
		return;	//return if NaN masked out

	//save chip state: DO loop status, a0-a3, dauc
	//not implemented yet
	DSP_reg_r[20] = reg_PC;
	reg_pcsh = reg_PC;	//save PC
	reg_PC = reg_evtp + 8*5;	//NaN opcode entry
	return;
}

/********************************************************/
/* NaN opcode exception                                 */
/*                                                      */
/********************************************************/
void DSP_exception_NaN()
{
	if(!(io_emr & 0x0040))
		return;	//return if NaN masked out

	//save chip state: DO loop status, a0-a3, dauc
	//not implemented yet
	DSP_reg_r[20] = reg_PC;
	reg_pcsh = reg_PC;	//save PC
	reg_PC = reg_evtp + 8*6;	//NaN opcode entry
	return;
}

/********************************************************/
/* Undefined behaviour handling                         */
/*                                                      */
/********************************************************/
void DSP_undefined()
{
	//certain instruction codes are marked as Reserved and yet
	//aren't listed in the manual as triggering the illegal
	//opcode exception.  It's not known what happens in all these
	//cases.  Here they are assumed to trigger the illegal opcode
	//exception.
	DSP_exception_illegal();
	return;
}

/********************************************************/
/* Memory read/write routines                           */
/********************************************************/
int32_t *dsp_check_mem(int32_t addr, int32_t *flag)
{
	if(DSP_io_r[12] & 0x0400) {
		//we are in memory map mode 1
		if(addr >= map1_ROM_start && addr <= map1_ROM_end) {
			//we are accessing ROM
			*flag = 1;
			return (int32_t *)(DSP_onboard_ROM + addr - map1_ROM_start);
		}
		if(addr >= map1_RAM_start && addr <= map1_RAM_end) {
			//we are accessing onboard RAM
			*flag = 2;
			return (int32_t *)(DSP_onboard_RAM + addr - map1_RAM_start);
		}
		if(addr >= map1_MMIO_start && addr <= map1_MMIO_end) {
			//access to MMIO space
			*flag = 4;
			return (int32_t *)(DSP_MMIO + addr - map1_MMIO_start);
		}
	} else {
		//we are in memory map mode 0
		if(addr >= map0_ROM_start && addr <= map0_ROM_end) {
			//we are accessing ROM
			*flag = 1;
			return (int32_t *)(DSP_onboard_ROM + addr - map0_ROM_start);
		}
		if(addr >= map0_RAM_start && addr <= map0_RAM_end) {
			//we are accessing onboard RAM
			*flag = 2;
			return (int32_t *)(DSP_onboard_RAM + addr - map0_RAM_start);
		}
		if(addr >= map0_MMIO_start && addr <= map0_MMIO_end) {
			//access to MMIO space
			*flag = 3;
			return (int32_t *)(DSP_MMIO + addr - map0_MMIO_start);
		}
	}
	*flag = 0;
	return NULL;
}

extern int log_dsp;
static const char *iotype[] = { "", "ROM", "RAM", "MMIO0", "MMIO1" };

int32_t DSP_get_long(int32_t addr)
{
	int32_t flag;
	int32_t *actual_addr = dsp_check_mem(addr, &flag);
	switch(flag)
	{
	case 0:			//read Amiga mem
		return DSP_get_long_ext(addr);

	default:		//read onboard ROM, RAM, MMIO
		if (log_dsp) {
			write_log("LGET from DSP %s: %08x = %08x\n", iotype[flag], addr, *actual_addr);
		}
		return *actual_addr;
	}
}

void DSP_set_long(int32_t addr, int32_t value)
{
	int32_t flag;
	int32_t *actual_addr = dsp_check_mem(addr, &flag);
	switch(flag)
	{
	case 0:
		DSP_set_long_ext(addr, value); //write to Amiga mem
		break;

	case 1:
		if (log_dsp) {
			write_log("LPUT DSP ROM %08x = %08x\n", addr, *actual_addr);
		}
		break;					//attempt to write to onboard ROM! Do nothing

	case 2:
		if (log_dsp) {
			write_log("LPUT DSP RAM %08x = %08x\n", addr, *actual_addr);
		}
		*actual_addr = value;	//write to onboard RAM
		break;

	case 3:
		if (log_dsp) {
			write_log("LPUT DSP MMIO0 %08x = %08x\n", addr, *actual_addr);
		}
		if(addr == (0x41C + map0_ROM_start))	//write to bio reg
			DSP_check_interrupt(value, 28);
		*actual_addr = value;
		break;

	case 4:
		if (log_dsp) {
			write_log("LPUT DSP MMIO1 %08x = %08x\n", addr, *actual_addr);
		}
		if(addr == (0x41C + map1_ROM_start))
			DSP_check_interrupt(value, 28);
		*actual_addr = value;
		break;
	}
}

int16_t DSP_get_short(int32_t addr)
{
	int32_t flag;
	int16_t *actual_addr = (int16_t *)dsp_check_mem(addr, &flag);

	switch(flag)
	{
	case 0:			//read Amiga mem
		return DSP_get_short_ext(addr);

	default:		//read onboard ROM, RAM, MMIO
		return *actual_addr;
	}
}

void DSP_set_short(int32_t addr, int16_t value)
{
	int32_t flag;
	int16_t *actual_addr = (int16_t *)dsp_check_mem(addr, &flag);

	switch(flag)
	{
	case 0:
		DSP_set_short_ext(addr, value); //write to Amiga mem
		break;

	case 1:					//attempt to write to onboard ROM! Do nothing
		if (log_dsp) {
			write_log("WPUT DSP ROM %08x = %04x\n", addr, *actual_addr);
		}
		break;

	case 2:					//write to onboard RAM
		if (log_dsp) {
			write_log("WPUT DSP RAM %08x = %04x\n", addr, *actual_addr);
		}
		*actual_addr = value;
		break;

	case 3:					//write to MMIO space
		if (log_dsp) {
			write_log("WPUT DSP MMIO0 %08x = %04x\n", addr, *actual_addr);
		}
		if(addr == 0x41E + map0_ROM_start)	//write to bio reg
			DSP_check_interrupt(value, 12-8);
		*actual_addr = value;
		break;

	case 4:					//write to MMIO space
		if (log_dsp) {
			write_log("WPUT DSP MMIO1 %08x = %04x\n", addr, *actual_addr);
		}
		if(addr == 0x41E + map1_ROM_start)
			DSP_check_interrupt(value, 12-8);
		*actual_addr = value;
		break;
	}
	return;
}

char DSP_get_char(int32_t addr)
{
	int32_t flag;
	char *actual_addr = (char *)dsp_check_mem(addr, &flag);

	switch(flag)
	{
	case 0:			//read Amiga mem
		return DSP_get_char_ext(addr);

	default:		//read onboard ROM, RAM, MMIO
		return *actual_addr;
	}
}

void DSP_set_char(int32_t addr, char value)
{
	int32_t flag;
	char *actual_addr = (char *)dsp_check_mem(addr, &flag);

	switch(flag)
	{
	case 0:					//write to Amiga mem
		DSP_set_char_ext(addr,value);
		break;

	case 1:					//attempt to write to onboard ROM! Do nothing
		if (log_dsp) {
			write_log("BPUT DSP ROM %08x = %02x\n", addr, *actual_addr);
		}
		break;

	case 2:					//write to onboard RAM
		if (log_dsp) {
			write_log("BPUT DSP RAM %08x = %02x\n", addr, *actual_addr);
		}
		*actual_addr = value;
		break;

	case 3:					//write to MMIO space
		if (log_dsp) {
			write_log("BPUT DSP MMIO0 %08x = %02x\n", addr, *actual_addr);
		}
		if(addr == 0x41F + map0_ROM_start)	//write to bio reg
			DSP_check_interrupt(value, 4);
		*actual_addr = value;
		break;

	case 4:					//write to MMIO space
		if (log_dsp) {
			write_log("BPUT DSP MMIO1 %08x = %02x\n", addr, *actual_addr);
		}
		if(addr == 0x41F + map1_ROM_start)
			DSP_check_interrupt(value, 4);
		*actual_addr = value;
		break;
	}
	return ;
}

void DSP_check_interrupt(int32_t value, int32_t bitno)
{
	int32_t mask;
	mask = 3 << bitno;		//test for bio[6] = INT6
	switch((value & mask) >> bitno)
	{
	case 0:		//no change to reg
		break;

	case 1:		//set reg to 0 = trigger int
		bio6 = 0;
		DSP_external_interrupt(1, 0);
		break;

	case 2:		//set reg to 1
		bio6 = 1;
		DSP_external_interrupt(1, 1);
		break;

	case 3:		//complement reg
		bio6 = (1-bio6);
		DSP_external_interrupt(1, bio6);
		break;
	}

	mask = 3 << (bitno + 2);	//test for bio[7] = INT2
	switch((value & mask) >> (bitno+2))
	{
	case 0:		//no change to reg
		break;

	case 1:		//set reg to 0 = trigger int
		bio7 = 0;
		DSP_external_interrupt(0, 0);
		break;

	case 2:		//set reg to 1
		bio7 = 1;
		DSP_external_interrupt(0, 1);
		break;

	case 3:		//complement reg
		bio7 = (1-bio7);
		DSP_external_interrupt(0, bio7);
		break;
	}
	return;
}

/********************************************************/
/* Onboard boot rom binary                              */
/********************************************************/

uint8_t DSP_onboard_ROM[] =
{
/* [0000][0001][0002][0003][0004][0005][0006][0007]*/
/*[0000]*/ 0x80, 0x2f, 0x00, 0x80, 0x9c, 0x61, 0x04, 0x0c,
/*[0008]*/ 0x80, 0x2f, 0x00, 0xec, 0x80, 0x00, 0x00, 0x00,
/*[0010]*/ 0x80, 0x2f, 0x00, 0xe4, 0x80, 0x00, 0x00, 0x00,
/*[0018]*/ 0x80, 0x2f, 0x00, 0xdc, 0x80, 0x00, 0x00, 0x00,
/*[0020]*/ 0x80, 0x2f, 0x00, 0xd4, 0x80, 0x00, 0x00, 0x00,
/*[0028]*/ 0x80, 0x2f, 0x00, 0xcc, 0x80, 0x00, 0x00, 0x00,
/*[0030]*/ 0x80, 0x2f, 0x00, 0xc4, 0x80, 0x00, 0x00, 0x00,
/*[0038]*/ 0x80, 0x2f, 0x00, 0xbc, 0x80, 0x00, 0x00, 0x00,
/*[0040]*/ 0x80, 0x2f, 0x00, 0xb4, 0x80, 0x00, 0x00, 0x00,
/*[0048]*/ 0x80, 0x2f, 0x00, 0xac, 0x80, 0x00, 0x00, 0x00,
/*[0050]*/ 0x80, 0x2f, 0x00, 0xa4, 0x80, 0x00, 0x00, 0x00,
/* [0058][0059][005a][005b][005c][005d][005e][005f]*/
/*[0058]*/ 0x80, 0x2f, 0x00, 0xa8, 0x80, 0x00, 0x00, 0x00,
/*[0060]*/ 0x80, 0x2f, 0x00, 0x94, 0x80, 0x00, 0x00, 0x00,
/*[0068]*/ 0x80, 0x2f, 0x00, 0x8c, 0x80, 0x00, 0x00, 0x00,
/*[0070]*/ 0x80, 0x2f, 0x00, 0x84, 0x80, 0x00, 0x00, 0x00,
/*[0078]*/ 0x9c, 0xfe, 0x00, 0x00, 0x9d, 0x60, 0x04, 0x08,
/*[0080]*/ 0x80, 0x3e, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
/*[0088]*/ 0x90, 0x40, 0x60, 0x00, 0x1b, 0xe1, 0x20, 0x00,
/*[0090]*/ 0x80, 0xaf, 0x00, 0x48, 0xc0, 0x03, 0xe0, 0x00,
/*[0098]*/ 0x8c, 0x00, 0x38, 0x02, 0x9c, 0xe4, 0x10, 0x17,
/*[00a0]*/ 0x80, 0x00, 0x00, 0x00, 0x9d, 0x04, 0x18, 0x17,
/*[00a8]*/ 0x1c, 0xe5, 0xe0, 0x04, 0x1c, 0xe1, 0xe0, 0x00,
/* [00b0][00b1][00b2][00b3][00b4][00b5][00b6][00b7]*/
/*[00b0]*/ 0x80, 0xaf, 0x00, 0x18, 0x98, 0x03, 0x08, 0x20,
/*[00b8]*/ 0x9a, 0x85, 0x00, 0x02, 0x9c, 0xe4, 0x10, 0x17,
/*[00c0]*/ 0x0c, 0xaf, 0xff, 0xf4, 0x9d, 0x04, 0x18, 0x17,
/*[00c8]*/ 0xa0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
/*[00d0]*/ 0x80, 0xaf, 0x00, 0x30, 0x80, 0x00, 0x00, 0x00,
/*[00d8]*/ 0xa0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
/*[00e0]*/ 0x93, 0x40, 0x50, 0x03, 0xc0, 0x02, 0x80, 0x00,
/*[00e8]*/ 0x9d, 0x62, 0x04, 0x08, 0x9b, 0xc1, 0xfb, 0xff,
/*[00f0]*/ 0x9d, 0xe0, 0x04, 0x0a, 0x9d, 0x61, 0x04, 0x0c,
/*[00f8]*/ 0x9d, 0xe0, 0x00, 0x00, 0x9d, 0x60, 0x04, 0x0a,
/*[0100]*/ 0x80, 0x2f, 0xff, 0xf8, 0x80, 0x00, 0x00, 0x00,
/* [0108][0109][010a][010b][010c][010d][010e][010f]*/
/*[0108]*/ 0x94, 0xcf, 0xfe, 0xf0, 0x95, 0x06, 0x03, 0xff,
/*[0110]*/ 0xc0, 0x07, 0xe0, 0x00, 0x98, 0x03, 0x30, 0x27,
/*[0118]*/ 0x9d, 0xe0, 0x18, 0x00, 0x30, 0x00, 0x0c, 0x07,
/*[0120]*/ 0x14, 0x40, 0x00, 0x01, 0x98, 0x24, 0x30, 0x20,
/*[0128]*/ 0x8c, 0x3f, 0xf8, 0x01, 0x49, 0xa8, 0x13, 0x9f,
/*[0130]*/ 0x99, 0xc4, 0x20, 0x28, 0x98, 0x73, 0x00, 0x20,
/*[0138]*/ 0x99, 0x0a, 0x30, 0x27, 0x8c, 0x07, 0xf8, 0x07,
/*[0140]*/ 0x79, 0x60, 0x2b, 0x87, 0x79, 0x40, 0x2b, 0x07,
/*[0148]*/ 0x9c, 0x74, 0x50, 0x17, 0x9c, 0x75, 0x50, 0x17,
/*[0150]*/ 0x34, 0x00, 0xc0, 0x07, 0x34, 0x00, 0x80, 0x07,
/*[0158]*/ 0x98, 0x13, 0x98, 0x34, 0x98, 0x13, 0x98, 0x35,
/* [0160][0161][0162][0163][0164][0165][0166][0167]*/
/*[0160]*/ 0x98, 0x85, 0x00, 0x20, 0x99, 0x4b, 0x00, 0x20,
/*[0168]*/ 0xdf, 0xf5, 0xff, 0xff, 0x9a, 0x8a, 0x04, 0x00,
/*[0170]*/ 0x8c, 0x3f, 0xf8, 0x06, 0x9c, 0xe9, 0x50, 0x00,
/*[0178]*/ 0x98, 0x05, 0x28, 0x2b, 0x98, 0xd4, 0x48, 0x29,
/*[0180]*/ 0x9d, 0xf4, 0x50, 0x00, 0x9c, 0xe6, 0x50, 0x17,
/*[0188]*/ 0x98, 0x05, 0x48, 0x25, 0x98, 0xcb, 0x30, 0x34,
/*[0190]*/ 0x95, 0x6a, 0xe0, 0x00, 0x7c, 0xa0, 0x00, 0x58,
/*[0198]*/ 0x1c, 0xf1, 0x04, 0x0c, 0x1c, 0xee, 0x04, 0x14,
/*[01a0]*/ 0x9c, 0x11, 0x04, 0x0e, 0x9c, 0xec, 0x58, 0x00,
/*[01a8]*/ 0x1c, 0xf2, 0x04, 0x20, 0x98, 0x8c, 0x60, 0x33,
/*[01b0]*/ 0x98, 0x0c, 0x60, 0x25, 0xa0, 0x01, 0x00, 0x00,
/* [01b8][01b9][01ba][01bb][01bc][01bd][01be][01bf]*/
/*[01b8]*/ 0x98, 0x02, 0xb0, 0xa2, 0x87, 0x64, 0x7f, 0x67,
};
