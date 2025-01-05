#ifndef M68_INTERNAL_H
#define M68_INTERNAL_H

#include <stdint.h>

//! Addressing modes
typedef enum {
	AMODE_DIRECT,
	AMODE_DIRECT_REL,
	AMODE_DIRECT_JUMP,
	AMODE_EXTENDED,
	AMODE_EXTENDED_JUMP,
	AMODE_IMMEDIATE,
	AMODE_INDEXED0,
	AMODE_INDEXED0_JUMP,
	AMODE_INDEXED1,
	AMODE_INDEXED1_JUMP,
	AMODE_INDEXED2,
	AMODE_INDEXED2_JUMP,
	AMODE_INHERENT,
	AMODE_INHERENT_A,
	AMODE_INHERENT_X,
	AMODE_RELATIVE,
	AMODE_ILLEGAL,
	AMODE_MAX
} M68_AMODE;


typedef struct M68_OPTABLE_ENT {
	char *			mnem;		/* instruction mnemonic */
	M68_AMODE		amode;		/* addressing mode */
	uint8_t			cycles;		/* number of cycles to execute */
	bool			write_only;	/* is write-only?  only supported for direct, extended and indexed */
	bool (*opfunc)(M68_CTX *ctx, const uint8_t opcode, uint8_t *param);	/* opcode exec function */
} M68_OPTABLE_ENT;


extern M68_OPTABLE_ENT m68hc05_optable[256];

// M68HC vector addresses.
static const uint16_t _M68_RESET_VECTOR = 0xFFFE;
static const uint16_t _M68_SWI_VECTOR   = 0xFFFC;
static const uint16_t _M68_INT_VECTOR   = 0xFFFA;
static const uint16_t _M68_TMR1_VECTOR  = 0xFFF8;

// TODO - need to have an m68_int function which allows vector address to be passed at runtime

void jump_to_vector(M68_CTX *ctx,uint16_t addr);



/**
 * Force one or more flag bits to a given state
 *
 * @param	ctx			Emulation context
 * @param	ccr_bits	CCR bit map (OR of M68_CCR_x constants)
 * @param	state		State to set -- true for set, false for clear
 */
static inline void force_flags(M68_CTX *ctx, const uint8_t ccr_bits, const bool state)
{
	if (state) {
		ctx->reg_ccr |= ccr_bits;
	} else {
		ctx->reg_ccr &= ~ccr_bits;
	}

	// Force CCR top 3 bits to 1
	ctx->reg_ccr |= 0xE0;
}

/**
 * Get the state of a CCR flag bit
 *
 * @param	ctx			Emulation context
 * @param	ccr_bits	CCR bit (M68_CCR_x constant)
 */
static inline bool get_flag(M68_CTX *ctx, const uint8_t ccr_bit)
{
	return (ctx->reg_ccr & ccr_bit) ? true : false;
}

/**
 * Push a byte onto the stack
 *
 * @param	ctx			Emulation context
 * @param	value		Byte to PUSH
 */
static inline void push_byte(M68_CTX *ctx, const uint8_t value)
{
	ctx->write_mem(ctx, ctx->reg_sp, value);
	ctx->reg_sp = ((ctx->reg_sp - 1) & ctx->sp_and) | ctx->sp_or;
}

/**
 * Pop a byte off of the stack
 *
 * @param	ctx			Emulation context
 * @return	Byte popped off of the stack
 */
static inline uint8_t pop_byte(M68_CTX *ctx)
{
	ctx->reg_sp = ((ctx->reg_sp + 1) & ctx->sp_and) | ctx->sp_or;
	return ctx->read_mem(ctx, ctx->reg_sp);
}



#endif // M68_INTERNAL_H
