#ifndef M68EMU_H
#define M68EMU_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	M68_CPU_HC05C4,
	M68_CPU_HD6805V1
} M68_CPUTYPE;

typedef enum{
  M68_INT_IRQ,
  M68_INT_TIMER1
} M68_INTERRUPT;


struct M68_CTX;

typedef uint8_t (*M68_READMEM_F)  (struct M68_CTX *ctx, const uint16_t addr);
typedef void    (*M68_WRITEMEM_F) (struct M68_CTX *ctx, const uint16_t addr, const uint8_t data);
typedef uint8_t (*M68_OPDECODE_F) (struct M68_CTX *ctx, const uint8_t value);


/**
 * Emulation context structure
 */
typedef struct M68_CTX {
	uint8_t			reg_acc;				///< Accumulator register
	uint8_t			reg_x;					///< X-index register
	uint16_t		reg_sp;					///< Stack pointer
	uint16_t		reg_pc;					///< Program counter for current instruction
	uint16_t		pc_next;				///< Program counter for next instruction
	uint8_t			reg_ccr;				///< Condition code register
	M68_CPUTYPE		cpuType;				///< CPU type
	bool			irq;					///< IRQ input state
        uint8_t                 pending_interrupts;
	uint16_t		sp_and, sp_or;			///< Stack pointer AND/OR masks
	uint16_t		pc_and;					///< Program counter AND mask
	bool			is_stopped;				///< True if processor is stopped
	bool			is_waiting;				///< True if processor is WAITing
	M68_READMEM_F	read_mem;				///< Memory read callback
	M68_WRITEMEM_F	write_mem;				///< Memory write callback
	M68_OPDECODE_F	opdecode;				///< Opcode decode function, or NULL
	bool			trace;
} M68_CTX;


/* CCR bits */
#define		M68_CCR_H	0x10		/* Half carry */
#define		M68_CCR_I	0x08		/* Interrupt mask */
#define		M68_CCR_N	0x04		/* Negative */
#define		M68_CCR_C	0x02		/* Carry/borrow */
#define		M68_CCR_Z	0x01		/* Zero */


void m68_init(M68_CTX *ctx, const M68_CPUTYPE cpuType);
void m68_reset(M68_CTX *ctx);
uint64_t m68_exec_cycle(M68_CTX *ctx);
void m68_set_interrupt_line(M68_CTX * ctx,M68_INTERRUPT i);


#endif // M68EMU_H
